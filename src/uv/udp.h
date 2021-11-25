#ifndef __LLAE_UV_UDP_H_INCLUDED__
#define __LLAE_UV_UDP_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "buffer.h"
#include "req.h"
#include <vector>

namespace uv {

    class loop;
    class udp;
    using udp_ptr = common::intrusive_ptr<udp>;

    class udp_send_req : public req {
    private:
        uv_udp_send_t    m_udp_send;
        udp_ptr  m_udp;
    protected:
        uv_loop_t* get_loop() { return m_udp_send.handle->loop; }
        static void send_cb(uv_udp_send_t* req, int status);
        virtual void on_send(int status) = 0;
        int start_send(const uv_buf_t* buffers,size_t count,const struct sockaddr* addr);
    public:
        udp_send_req(udp_ptr&& u);
        ~udp_send_req();
    };
    using udp_send_req_ptr = common::intrusive_ptr<udp_send_req>;

    class udp_send_lua_req : public udp_send_req {
    private:
        lua::ref m_cont;
        write_buffers m_buffers;
    protected:
        virtual void on_send(int status) override;
    public:
        udp_send_lua_req(udp_ptr&& stream,lua::ref&& cont);
        bool put(lua::state& s);
        bool empty() const { return m_buffers.empty(); }
        void reset(lua::state& s);
        int send(const struct sockaddr* addr);
    };
    using udp_send_lua_req_ptr = common::intrusive_ptr<udp_send_lua_req>;

    class udp_recv_consumer : public meta::object {
    public:
        virtual bool on_recv(udp* s,ssize_t nread, buffer_ptr&& buffer,const struct sockaddr* addr, unsigned flags) = 0;
        virtual void on_udp_closed(udp* s) {}
        virtual void on_stop_recv(udp* s) {}
    };
    typedef common::intrusive_ptr<udp_recv_consumer> udp_recv_consumer_ptr;

    class udp : public handle {
        META_OBJECT
    private:
        uv_udp_t m_udp;
        udp_recv_consumer_ptr m_recv_consumer;
        bool m_connected = false;
        static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
        static void recv_cb(uv_udp_t* stream, ssize_t nread, const uv_buf_t* buf,const struct sockaddr* addr, unsigned flags);
        std::vector<uv::buffer_ptr> m_buffers;
    public:
        virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_udp); }
        uv_udp_t* get_udp() { return &m_udp; }
    protected:
        virtual ~udp() override;
        virtual void on_closed() override;
        bool on_recv(ssize_t nread, buffer_ptr&& buffer,const struct sockaddr* addr, unsigned flags);
    public:
        explicit udp(uv::loop& loop);
        
        static lua::multiret lnew(lua::state& l);
        static void lbind(lua::state& l);

        int do_bind(struct sockaddr* addr,int flags);
        int do_set_broadcast(bool b);
        int do_set_membership(const char *multicast_addr, const char *interface_addr, uv_membership membership);
        int do_set_multicast_loop(bool on);
        int do_set_multicast_ttl(int ttl);
        int do_try_send(const buffer_ptr& buffer,const struct sockaddr *addr);
        
        lua::multiret bind(lua::state& l);
        lua::multiret send(lua::state& l);
        lua::multiret try_send(lua::state& l);
        lua::multiret connect(lua::state& l);
        void add_buffer(uv::buffer_ptr&& buffer);
        void disconnect();
        lua::multiret recv(lua::state& l);
        int start_recv( const udp_recv_consumer_ptr& consumer );
        void stop_recv();
    };

}

#endif /*__LLAE_UV_UDP_H_INCLUDED__*/

