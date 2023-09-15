#include "redis.h"
#include "lua/bind.h"
#include "lua/stack.h"
#include "uv/tcp_connection.h"
#include "uv/buffer.h"
#include "llae/app.h"
#include "uv/luv.h"
#include <cmath>
#include <cstdlib>
#include <cinttypes>

META_OBJECT_INFO(db::resp3,meta::object)

namespace db {

    static inline size_t number_len(size_t number) {
      return number == 0 ? 1 : (std::log10(number) + 1);
    }

    resp3::resp3( uv::stream_ptr&& stream ) : m_stream(std::move(stream)) {

    }

    uv::buffer_ptr resp3::encode(lua::state& l,int start_idx) {
        int top = l.gettop();
        if (top < start_idx) {
            l.error("need command");
        }
        if (top > 9999) {
            l.error("too many args");
        }
        auto res = uv::buffer::alloc(1+number_len(top-start_idx+1)+2+2+(top-start_idx+1)*16); // euristic
        res->set_len(0);
        auto require_size = [&res](size_t size) {
            while ((res->get_len()+size) > res->get_capacity()) {
                res = res->realloc(res->get_capacity()*3/2);
            }
        };
        auto write_ch = [&res,&require_size](char c) {
            require_size(0);
            static_cast<char*>(res->get_base())[res->get_len()] = c;
            res->set_len(res->get_len()+1);
        };
        auto write_rn = [&res,&require_size]() {
            require_size(2);
            static_cast<char*>(res->get_base())[res->get_len()] = '\r';
            static_cast<char*>(res->get_base())[res->get_len()+1] = '\n';
            res->set_len(res->get_len()+2);
        };
        auto write_data = [&res,&require_size](const void* data,size_t size) {
            require_size(size);
            memcpy(static_cast<char*>(res->get_base())+res->get_len(),data,size);
            res->set_len(res->get_len()+size);
        };
        auto write_length = [&res,&require_size](size_t number) {
            char buf[16];
            ::snprintf(buf,sizeof(buf)-1,"%zu",number);
            size_t sz = ::strlen(buf);
            require_size(sz);
            memcpy(static_cast<char*>(res->get_base())+res->get_len(),buf,sz);
            res->set_len(res->get_len()+sz);
        };
        auto write_numberi = [&res,&require_size](lua_Integer number) {
            char buf[16];
            ::snprintf(buf,sizeof(buf)-1,"%" PRId64,number);
            size_t sz = ::strlen(buf);
            require_size(sz);
            memcpy(static_cast<char*>(res->get_base())+res->get_len(),buf,sz);
            res->set_len(res->get_len()+sz);
        };
        write_ch('*');
        write_length(top-start_idx+1);
        write_rn();
        for (int i=start_idx;i<=top;++i) {
            auto t = l.get_type(i);
            if (t == lua::value_type::lnil) {
                write_ch('_');
            } else if (t == lua::value_type::string) {
                size_t sz = 0;
                auto str = l.tolstring(i,sz);
                write_ch('$');
                write_length(sz);
                write_rn();
                write_data(str,sz);
            } else if (t == lua::value_type::userdata) {
                auto buf = uv::buffer_base::get(l,i,true);
                if (!buf) {
                    l.argerror(i,"invalid type");
                } else {
                    write_ch('$');
                    write_length(buf->get_len());
                    write_rn();
                    write_data(buf->get_base(),buf->get_len());
                }
            } else if (l.isinteger(i)) {
                write_ch(':');
                write_numberi(l.tointeger(i));
            } else if (t == lua::value_type::number) {
                l.pushvalue(i);
                size_t sz = 0;
                auto str = l.tolstring(-1,sz);
                write_ch(',');
                write_data(str,sz);
                l.pop(1);
            } else if (t == lua::value_type::boolean) {
                write_ch('#');
                write_ch(l.toboolean(i)?'t':'f');
            } else {
                l.argerror(i,"unexpected type");
            }
            write_rn();
        }
        return res;
    }

    class resp3::write_buffer_req : public uv::write_buffer_req {
    private:
        resp3_ptr m_self;
    public:
        write_buffer_req(resp3_ptr&& self,uv::buffer_ptr&& data) : uv::write_buffer_req(uv::stream_ptr(self->get_stream()),std::move(data)), m_self(std::move(self)) {
        }
        virtual void on_write(int status) override {
            auto& l{llae::app::get(get_loop()).lua()};
            m_self->on_write_complete(l,status);
            m_self.reset();
            uv::write_buffer_req::on_write(0);
        }
    };

    class resp3::write_lua_req : public uv::write_req {
    private:
        resp3_ptr m_self;
        uv::write_buffers m_buffers;
    public:
        explicit write_lua_req(resp3_ptr&& self) : uv::write_req(uv::stream_ptr(self->get_stream())), m_self(std::move(self)) {
        }
        void reset(lua::state& l) {
            m_buffers.reset(l);
        }
        virtual void on_write(int status) override {
            auto& l = llae::app::get(get_loop()).lua();
            if (!l.native()) {
                m_buffers.release();
            } else {
                m_buffers.reset(l);
            }
            m_self->on_write_complete(l,status);
            m_self.reset();
        }
        bool put(lua::state& l) {
            return m_buffers.put(l);
        }
        int write() {
            return start_write(m_buffers.get_buffers().data(),m_buffers.get_buffers().size());
        }
    };

    void resp3::on_write_complete(lua::state& l,int status) {
        if (!m_stream) {
            return;
        }
        if (!l.native()) {
            m_cont.release();
            return;
        }
        if (m_cont.valid()) {
            auto th = std::move(m_cont);
            th.push(l);
            auto toth = l.tothread(-1);
            int args;
            if (status < 0) {
                toth.pushnil();
                uv::push_error(toth,status);
                args = 2;
            } else {
                toth.pushboolean(true);
                args = 1;
            }
            auto s = toth.resume(l,args);
            if (s != lua::status::ok && s != lua::status::yield) {
                llae::app::show_error(toth,s);
            }
            th.reset(l);
        }
    }

    lua::multiret resp3::cmd(lua::state& l) {
        if (!m_stream) {
            l.pushnil();
            l.pushstring("resp3::pipelining closed");
            return {2};
        }
        auto data = encode(l,2);
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("resp3::cmd is async");
            return {2};
        }
        if (m_cont.valid()) {
            l.pushnil();
            l.pushstring("resp3::cmd busy");
            return {2};
        }
        {
            l.pushthread();
            m_cont.set(l);

            common::intrusive_ptr<write_buffer_req> req{new write_buffer_req(resp3_ptr(this),std::move(data))};
        
            int r = req->write();
            if (r < 0) {
                l.pushnil();
                uv::push_error(l,r);
                return {2};
            } 
        }
        l.yield(0);
        return {0};
    }

    lua::multiret resp3::pipelining(lua::state& l) {
        if (!m_stream) {
            l.pushnil();
            l.pushstring("resp3::pipelining closed");
            return {2};
        }
        
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("resp3::pipelining is async");
            return {2};
        }
        if (m_cont.valid()) {
            l.pushnil();
            l.pushstring("resp3::pipelining busy");
            return {2};
        }
        {
            

            common::intrusive_ptr<write_lua_req> req{new write_lua_req(resp3_ptr(this))};

            l.pushvalue(2);
            if (!req->put(l)) {
                req->reset(l);
                l.pushnil();
                l.pushstring("resp3::pipelining invalid data");
                return {2};
            }

            l.pushthread();
            m_cont.set(l);
        
            int r = req->write();
            if (r < 0) {
                req->reset(l);
                l.pushnil();
                uv::push_error(l,r);
                return {2};
            } 
        }
        l.yield(0);
        return {0};
    }

    lua::multiret resp3::gen_req(lua::state& l) {
        auto data = encode(l,1);
        lua::push(l,std::move(data));
        return {1};
    }

    bool resp3::on_read(uv::readable_stream* s,ssize_t nread, uv::buffer_ptr& buffer) {
        return true;
    }

    lua::multiret resp3::lnew(lua::state& l) {
        auto stream = lua::stack<uv::stream_ptr>::get(l,1);
        if (!stream) {
            l.argerror(1,"need stream");
        }
        common::intrusive_ptr<resp3> res{new resp3(std::move(stream))};
        lua::push(l,std::move(res));
        return {1};
    }

    lua::multiret resp3::close(lua::state& l) {
        if (m_cont.valid()) {
            l.pushnil();
            l.pushstring("resp3::pipelining busy");
            return {2};
        }
        m_stream.reset();
        l.pushboolean(true);
        return {1};
    }

    void resp3::lbind(lua::state& l) {
        lua::bind::function(l,"new",&resp3::lnew);
        lua::bind::function(l,"cmd",&resp3::cmd);
        lua::bind::function(l,"gen_req",&resp3::gen_req);
        lua::bind::function(l,"close",&resp3::close);
        lua::bind::function(l,"pipelining",&resp3::pipelining);
    }

}



int luaopen_db_redis_native(lua_State* L) {
    lua::state l(L);
    lua::bind::object<db::resp3>::register_metatable(l,&db::resp3::lbind);
    l.createtable();
    lua::bind::object<db::resp3>::get_metatable(l);
    l.setfield(-2,"resp3");

    return 1;
}

