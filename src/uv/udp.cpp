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
                             buffer_ptr&& buffer,
                             const struct sockaddr* addr, unsigned flags) override final {
            auto& l = llae::app::get(s->get_udp()->loop).lua();
            if (m_recv_cont.valid()) {
                m_recv_cont.push(l);
                auto toth = l.tothread(-1);
                l.pop(1); // toth holded at m_recv_cont
                int args;
                if (nread >= 0) {
                    buffer->set_len(nread);
                    lua::push(toth,std::move(buffer));
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
        int res = do_bind((struct sockaddr*)&addr,flags);
        if (res < 0) {
            l.pushnil();
            uv::push_error(l,res);
            return {2};
        }
        l.pushboolean(true);
        return {1};
    }

    int udp::do_bind(struct sockaddr* addr,int flags) {
        return uv_udp_bind(&m_udp,addr,flags);
    }
    int udp::do_set_ttl(int ttl) {
        return uv_udp_set_ttl(&m_udp,ttl);
    }
    int udp::do_set_broadcast(bool b) {
        return uv_udp_set_broadcast(&m_udp,b?1:0);
    }
    int udp::do_set_membership(const char *multicast_addr, const char *interface_addr, uv_membership membership) {
        return uv_udp_set_membership(&m_udp, multicast_addr, interface_addr, membership);
    }
    int udp::do_set_source_membership(const char *multicast_addr, const char *interface_addr, 
            const char* source_addr, uv_membership membership) {
        return uv_udp_set_source_membership(&m_udp, multicast_addr, interface_addr, source_addr, membership);
    }
    int udp::do_set_multicast_loop(bool on) {
        return uv_udp_set_multicast_loop(&m_udp,on);
    }
    int udp::do_set_multicast_ttl(int ttl) {
        return uv_udp_set_multicast_ttl(&m_udp,ttl);
    }
    int udp::do_set_multicast_interface(const char* interface_addr) {
        return uv_udp_set_multicast_interface(&m_udp,interface_addr);
    }

    lua::multiret udp::set_ttl(lua::state& l,int ttl) {
        auto res = do_set_ttl(ttl);
        return return_status_error(l,res);
    }

    lua::multiret udp::set_broadcast(lua::state& l,bool b) {
        auto res = do_set_broadcast(b);
        return return_status_error(l,res);
    }
    
    lua::multiret udp::set_membership(lua::state& l,const char *multicast_addr, const char *interface_addr, uv_membership membership) {
        auto res = do_set_membership(multicast_addr,interface_addr,membership);
        return return_status_error(l,res);
    }

    lua::multiret udp::set_source_membership(lua::state& l,const char *multicast_addr, const char *interface_addr, 
            const char* source_addr, uv_membership membership) {
        auto res = do_set_source_membership(multicast_addr,interface_addr,source_addr,membership);
        return return_status_error(l,res);
    }
    
    lua::multiret udp::set_multicast_loop(lua::state& l,bool on) {
        auto res = do_set_multicast_loop(on);
        return return_status_error(l,res);
    }
    
    lua::multiret udp::set_multicast_ttl(lua::state& l,int ttl) {
        auto res = do_set_multicast_ttl(ttl);
        return return_status_error(l,res);
    }

    lua::multiret udp::set_multicast_interface(lua::state& l,const char* interface_addr) {
        auto res = do_set_multicast_interface(interface_addr);
        return return_status_error(l,res);
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
        udp* self = static_cast<udp*>(handle->data);
        uv::buffer_ptr b;
        if (!self->m_buffers.empty()) {
            b = std::move(self->m_buffers.back());
            self->m_buffers.pop_back();
        } else {
            b = buffer::alloc(suggested_size);
        }
        buf->base = static_cast<char*>(b->get_base());
        buf->len = b->get_capacity();
        b->add_ref();
    }

    void udp::add_buffer(uv::buffer_ptr&& buffer) {
        if (buffer) {
            m_buffers.emplace_back(std::move(buffer));
        } else {
            
        }
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

    bool udp::on_recv(ssize_t nread, buffer_ptr&& buffer,const struct sockaddr* addr, unsigned flags) {
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
    int udp::do_try_send(const buffer_ptr& buffer,const struct sockaddr *addr) {
        return uv_udp_try_send(get_udp(),buffer->get(),1,addr);
    }

    lua::multiret udp::try_send(lua::state& l) {
        struct sockaddr_storage addr;
        bool with_addr = false;
        if (l.gettop()>2) {
            const char* host = l.checkstring(3);
            int port = l.checkinteger(4);
            if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
                  uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
                l.error("udp::try_send invalid IP address or port [%s:%d]", host, port);
            }
            with_addr = true;
        }
        auto buf = buffer::get(l,2);
        if (!buf) {
            l.argerror(2,"need buffer");
        }
        auto r = do_try_send(buf,(struct sockaddr*)(with_addr ? &addr : nullptr));
        if (r < 0) {
            l.pushnil();
            uv::push_error(l,r);
            return {2};
        } else {
            l.pushinteger(r);
            return {1};
        }
    }

    lua::multiret udp::getpeername(lua::state& l) {
        struct sockaddr_storage addr;
        int namelen = sizeof(addr);
        auto r = uv_udp_getpeername(&m_udp,(struct sockaddr*)&addr,&namelen);
        if (r < 0) {
            l.pushnil();
            uv::push_error(l,r);
            return {2};
        }
        char name[64];
        if (uv_ip4_name((const struct sockaddr_in*)&addr,name,sizeof(name)) == 0) {
            l.pushstring(name);
            l.pushinteger(ntohs(((const struct sockaddr_in*)&addr)->sin_port));
        } else if (uv_ip6_name((const struct sockaddr_in6*)&addr,name,sizeof(name))==0) {
            l.pushstring(name);
            l.pushinteger(ntohs(((const struct sockaddr_in6*)&addr)->sin6_port));
        } else {
            l.pushnil();
            l.pushstring("unknown");
            return {2};
        }
        return {2};
    }

    lua::multiret udp::getsockname(lua::state& l) {
        struct sockaddr_storage addr;
        int namelen = sizeof(addr);
        auto r = uv_udp_getpeername(&m_udp,(struct sockaddr*)&addr,&namelen);
        if (r < 0) {
            l.pushnil();
            uv::push_error(l,r);
            return {2};
        }
        char name[64];
        if (uv_ip4_name((const struct sockaddr_in*)&addr,name,sizeof(name)) == 0) {
            l.pushstring(name);
            l.pushinteger(ntohs(((const struct sockaddr_in*)&addr)->sin_port));
        } else if (uv_ip6_name((const struct sockaddr_in6*)&addr,name,sizeof(name))==0) {
            l.pushstring(name);
            l.pushinteger(ntohs(((const struct sockaddr_in6*)&addr)->sin6_port));
        } else {
            l.pushnil();
            l.pushstring("unknown");
            return {2};
        }
        return {2};
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
        lua::bind::function(l,"try_send",&udp::try_send);
        lua::bind::function(l,"recv",&udp::recv);
        lua::bind::function(l,"connect",&udp::connect);
        lua::bind::function(l,"disconnect", &udp::disconnect);
        lua::bind::function(l,"stop_recv",&udp::stop_recv);
        lua::bind::function(l,"add_buffer",&udp::add_buffer);
        lua::bind::function(l,"close",&udp::close);
        lua::bind::function(l,"set_ttl",&udp::set_ttl);
        lua::bind::function(l,"set_broadcast",&udp::set_broadcast);
        lua::bind::function(l,"set_membership",&udp::set_membership);
        lua::bind::function(l,"set_source_membership",&udp::set_source_membership);
        lua::bind::function(l,"set_multicast_loop",&udp::set_multicast_loop);
        lua::bind::function(l,"set_multicast_ttl",&udp::set_multicast_ttl);
        lua::bind::function(l,"set_multicast_interface",&udp::set_multicast_interface);
        lua::bind::function(l,"getpeername",&udp::getpeername);
        lua::bind::function(l,"getsockname",&udp::getsockname);

        l.pushinteger(UV_UDP_IPV6ONLY);
        l.setfield(-2,"IPV6ONLY");
        l.pushinteger(UV_UDP_REUSEADDR);
        l.setfield(-2,"REUSEADDR");
        l.pushinteger(UV_UDP_PARTIAL);
        l.setfield(-2,"PARTIAL");
        l.pushinteger(UV_LEAVE_GROUP);
        l.setfield(-2,"LEAVE_GROUP");
        l.pushinteger(UV_JOIN_GROUP);
        l.setfield(-2,"JOIN_GROUP");
    }
}
