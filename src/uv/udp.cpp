#include "udp.h"
#include "llae/app.h"
#include "luv.h"
#include "lua/bind.h"
#include "lua/stack.h"
#include <iostream>

META_OBJECT_INFO(uv::udp,uv::handle)

namespace uv {


    udp_send_req::udp_send_req(udp_ptr&& s) : m_udp(std::move(s)) {
        attach(reinterpret_cast<uv_req_t*>(&m_udp_send));
    }
    udp_send_req::~udp_send_req() {
    }

    void udp_send_req::send_cb(uv_udp_send_t* req, int status) {
        udp_send_req* self = static_cast<udp_send_req*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
        self->on_send(status);
        self->remove_ref();
    }

    int udp_send_req::start_send(const uv_buf_t* buffers,size_t count, const struct sockaddr* addr) {
        add_ref();
        int r = uv_udp_send(&m_udp_send,m_udp->get_udp(),
                         buffers,count,addr,&udp_send_req::send_cb);
        if (r < 0) {
            remove_ref();
        }
        return r;
    }


    udp_send_lua_req::udp_send_lua_req(udp_ptr&& s,lua::ref&& cont) : udp_send_req(std::move(s)),m_cont(std::move(cont)) {
    }

    void udp_send_lua_req::on_send(int status) {
        auto& l = llae::app::get(get_loop()).lua();
        m_buffers.reset(l);
        if (m_cont.valid()) {
            m_cont.push(l);
            auto toth = l.tothread(-1);
            l.pop(1);// thread
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
            m_cont.reset(l);
        }
    }

    void udp_send_lua_req::reset(lua::state& l) {
        m_buffers.reset(l);
        m_cont.reset(l);
    }

    bool udp_send_lua_req::put(lua::state& l) {
        return m_buffers.put(l);
    }

    int udp_send_lua_req::send(const struct sockaddr* addr) {
        return start_send(m_buffers.get_buffers().data(),m_buffers.get_buffers().size(),addr);
    }


    class lua_recv_consumer : public udp_recv_consumer {
    private:
        lua::ref m_recv_cont;
    public:
        lua_recv_consumer(lua::ref && cont) : m_recv_cont(std::move(cont)) {}
        virtual bool on_recv(udp* s,
                             ssize_t nread,
                             const buffer_ptr&& buffer,
                             const struct sockaddr* addr, unsigned flags) override final {
            auto& l = llae::app::get(s->get_udp()->loop).lua();
            if (m_recv_cont.valid()) {
                m_recv_cont.push(l);
                auto toth = l.tothread(-1);
                l.pop(1); // toth holded at m_recv_cont
                int args;
                if (nread >= 0) {
                    buffer->set_len(nread);
                    lua::push(toth,buffer);
                    toth.pushnil();
                    args = 2;
                    if (addr) {
                        char name[64];
                        if (uv_ip4_name((const struct sockaddr_in*)addr,name,sizeof(name)) == 0) {
                            toth.pushstring(name);
                            toth.pushinteger(ntohs(((const struct sockaddr_in*)addr)->sin_port));
                            args = 4;
                        } else if (uv_ip6_name((const struct sockaddr_in6*)addr,name,sizeof(name))==0) {
                            toth.pushstring(name);
                            toth.pushinteger(ntohs(((const struct sockaddr_in6*)addr)->sin6_port));
                            args = 4;
                        } 
                    }
                } else if (nread < 0) {
                    toth.pushnil();
                    uv::push_error(toth,nread);
                    args = 2;
                } 
                lua::ref ref(std::move(m_recv_cont));
                s->stop_recv();
                auto s = toth.resume(l,args);
                
                if (s != lua::status::ok && s != lua::status::yield) {
                    llae::app::show_error(toth,s);
                }
                ref.reset(l);
            } else {
                s->stop_recv();
            }
            return true;
        }
        virtual void on_stop_recv(udp* s) override {
            if (m_recv_cont.valid()) {
                auto& l = llae::app::get(s->get_udp()->loop).lua();
                m_recv_cont.reset(l);
            }
        }
    };


    udp::udp(uv::loop& loop) {
        int r = uv_udp_init(loop.native(),&m_udp);
        UV_DIAG_CHECK(r);
        attach();
    }
    udp::~udp() {
    }

    void udp::on_closed() {
        
    }

    lua::multiret udp::bind(lua::state& l) {
        const char* host = l.checkstring(2);
        int port = l.checkinteger(3);
        int flags = l.optinteger(4,0);
        struct sockaddr_storage addr;
        if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
              uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
            l.error("invalid IP address or port [%s:%d]", host, port);
           }
        int res = uv_udp_bind(&m_udp,(struct sockaddr*)&addr,flags);
        if (res < 0) {
            l.pushnil();
            uv::push_error(l,res);
            return {2};
        }
        l.pushboolean(true);
        return {1};
    }


    lua::multiret udp::send(lua::state& l) {
        struct sockaddr_storage addr;
        bool with_addr = false;
        if (l.gettop()>2) {
            const char* host = l.checkstring(3);
            int port = l.checkinteger(4);
            if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
                  uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
                l.error("udp::send invalid IP address or port [%s:%d]", host, port);
            }
            with_addr = true;
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("udp::send is async");
            return {2};
        }
        {
            l.pushthread();
            lua::ref send_cont;
            send_cont.set(l);

            udp_send_lua_req_ptr req{new udp_send_lua_req(udp_ptr(this),std::move(send_cont))};
        
            l.pushvalue(2);
            if (!req->put(l)) {
                req->reset(l);
                l.pushnil();
                l.pushstring("udp::send invalid data");
                return {2};
            }
            if (req->empty()) {
                req->reset(l);
                l.pushboolean(true);
                return {1};
            }

            int r = req->send((struct sockaddr*)(with_addr ? &addr : nullptr));
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

    lua::multiret udp::connect(lua::state& l) {
        struct sockaddr_storage addr;
        const char* host = l.checkstring(2);
        int port = l.checkinteger(3);
        if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
              uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
            l.error("invalid IP address or port [%s:%d]", host, port);
        }
        int r = uv_udp_connect(&m_udp,(struct sockaddr*)&addr);
        if (r < 0) {
            l.pushnil();
            uv::push_error(l,r);
            return {2};
        }
        m_connected = true;
        l.pushboolean(true);
        return {1};
    }

    void udp::disconnect() {
        int r = uv_udp_connect(&m_udp,nullptr);
        UV_DIAG_CHECK(r);
    }

    lua::multiret udp::recv(lua::state& l) {
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("udp:recv is async");
            return {2};
        }
        if (m_recv_consumer) {
            l.pushnil();
            l.pushstring("udp:recv already recv");
            return {2};
        }
        
        {
            l.pushthread();
            lua::ref recv_cont;
            recv_cont.set(l);

            common::intrusive_ptr<lua_recv_consumer> consume(new lua_recv_consumer(std::move(recv_cont)));
            int res = start_recv(consume);
            if (res < 0) {
                l.pushnil();
                uv::push_error(l,res);
                return {2};
            }
            
        }
        l.yield(0);
        return {0};
    }

    void udp::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        auto b = buffer::alloc(suggested_size);
        buf->base = static_cast<char*>(b->get_base());
        buf->len = b->get_capacity();
        b->add_ref();
    }

    void udp::recv_cb(uv_udp_t* u, ssize_t nread, const uv_buf_t* buf,const struct sockaddr* addr, unsigned flags) {
        udp* self = static_cast<udp*>(u->data);
        auto b = buffer::get(buf->base);
        if (self->on_recv(nread,buffer_ptr(b),addr,flags)) {
            //std::cout << "stream read_cb remove_ref" << std::endl;
            self->remove_ref();
        }
        if (b) {
            b->remove_ref();
        }
    }

    bool udp::on_recv(ssize_t nread, const buffer_ptr&& buffer,const struct sockaddr* addr, unsigned flags) {
        if (m_recv_consumer) {
            auto consumer = std::move(m_recv_consumer);
            bool res = consumer->on_recv(this,
                                                nread, std::move(buffer),addr,flags);
            if (!res) {
                m_recv_consumer = std::move(consumer);
            }
            return res;
        } else {
            LLAE_DIAG(std::cout << "recv without consumer" << std::endl;)
            stop_recv();
        }
        return true;
    }

    int udp::start_recv( const udp_recv_consumer_ptr& consumer ) {
        if (m_recv_consumer) {
            return -1;
        }
        m_recv_consumer = consumer;
        //std::cout << "stream start_read add_ref" << std::endl;
        add_ref();
        int res = uv_udp_recv_start(get_udp(), &udp::alloc_cb, &udp::recv_cb);
        if (res < 0) {
            m_recv_consumer.reset();
            //std::cout << "stream start_read error remove_ref" << std::endl;
            remove_ref();
        }
        return res;
    }

    void udp::stop_recv() {
        uv_udp_recv_stop(get_udp());
        if (m_recv_consumer) {
            m_recv_consumer->on_stop_recv(this);
        }
        m_recv_consumer.reset();
    }

    lua::multiret udp::lnew(lua::state& l) {
        common::intrusive_ptr<udp> server{new udp(llae::app::get(l).loop())};
        lua::push(l,std::move(server));
        return {1};
    }
    void udp::lbind(lua::state& l) {
        lua::bind::function(l,"new",&udp::lnew);
        lua::bind::function(l,"bind",&udp::bind);
        lua::bind::function(l,"send",&udp::send);
        lua::bind::function(l,"recv",&udp::recv);
        lua::bind::function(l,"connect",&udp::connect);
        lua::bind::function(l,"disconnect", &udp::disconnect);
        lua::bind::function(l,"stop_recv",&udp::stop_recv);
        lua::bind::function(l,"close",&udp::close);
        
        l.pushinteger(UV_UDP_IPV6ONLY);
        l.setfield(-2,"IPV6ONLY");
        l.pushinteger(UV_UDP_REUSEADDR);
        l.setfield(-2,"REUSEADDR");
    }
}
