#ifndef __LLAE_UV_UDP_H_INCLUDED__
#define __LLAE_UV_UDP_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "llae/buffer.h"
#include "llae/write_buffers.h"
#include "req.h"
#include "net/address.h"
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

    class udp_recv_consumer : public meta::object {
    public:
        virtual bool on_recv(udp* s,ssize_t nread, llae::buffer_ptr&& buffer,const net::address& addr, unsigned flags) = 0;
        virtual void on_udp_closed(udp* s) {}
        virtual void on_stop_recv(udp* s) {}
    };
    typedef common::intrusive_ptr<udp_recv_consumer> udp_recv_consumer_ptr;

    /// A received datagram with its source address.
    struct datagram {
        llae::buffer_ptr data;
        net::address address;
    };

    /// @luabind
    /// @luabind_include("../net/address_lua.h")
    class udp : public handle {
        META_OBJECT
    private:
        uv_udp_t m_udp;
        udp_recv_consumer_ptr m_recv_consumer;
        bool m_connected = false;
        static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
        static void recv_cb(uv_udp_t* stream, ssize_t nread, const uv_buf_t* buf,const struct sockaddr* addr, unsigned flags);
        std::vector<llae::buffer_ptr> m_buffers;
    public:
        virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_udp); }
        uv_udp_t* get_udp() { return &m_udp; }
    protected:
        virtual ~udp() override;
        virtual void on_closed() override;
        bool on_recv(ssize_t nread, llae::buffer_ptr&& buffer,const net::address& addr, unsigned flags);
    public:
        explicit udp(uv::loop& loop,int af);
        
        /// @luabind(name=new)
        static lua::multiret lnew(lua::state& l);
    
        int do_bind(const net::address& addr,int flags);
        int do_connect(const net::address& addr);
        int do_set_ttl(int ttl);
        int do_set_broadcast(bool b);
        int do_set_membership(const char *multicast_addr, const char *interface_addr, uv_membership membership);
        int do_set_source_membership(const char *multicast_addr, const char *interface_addr, 
            const char* source_addr, uv_membership membership);
        int do_set_multicast_loop(bool on);
        int do_set_multicast_ttl(int ttl);
        int do_set_multicast_interface(const char* interface_addr);

        /// Binds the socket to an address. `flags` is a combination of `udp.*` constants.
        /// @luabind(wrapper=uv::lua_udp_bind)
        /// @lparam(address,string|net.address)
        /// @lparam(port,integer?)
        /// @lparam(flags,integer?)
        /// @lreturn(result,boolean?)
        /// @lreturn(error,string?)
        llae::result<void> bind(const net::address& addr, std::optional<int> flags);

        /// Sends one or more buffers to `address` (or to the connected peer).
        /// @luabind(name=send,async=true,wrapper=uv::lua_udp_send)
        /// @lparam(data,string|llae.buffer_base|(string|llae.buffer_base)[])
        /// @lparam(host,string|net.address?)
        /// @lparam(port,integer?)
        /// @lreturn(result,boolean?)
        /// @lreturn(error,string?)
        llae::result_promise_ptr<void> async_send(llae::loop& l, llae::write_buffers&& buffers,
            std::optional<net::address> addr);

        /// Sends a buffer without queuing; returns the number of bytes sent.
        /// @luabind
        /// @lparam(data,string|llae.buffer_base)
        /// @lparam(host,string|net.address?)
        /// @lparam(port,integer?)
        /// @lreturn(result,integer?)
        /// @lreturn(error,string?)
        llae::result<int> try_send(llae::buffer_view data, std::optional<net::address> addr);

        /// Connects the socket to a fixed peer address.
        /// @luabind
        /// @lparam(address,string|net.address)
        /// @lparam(port,integer?)
        /// @lreturn(result,boolean?)
        /// @lreturn(error,string?)
        llae::result<void> connect(const net::address& addr);

        /// @luabind
        llae::result<void> set_ttl(int ttl);
        /// @luabind
        llae::result<void> set_broadcast(bool enable);
        /// @luabind
        /// @lparam(multicast_addr,string)
        /// @lparam(interface_addr,string?)
        /// @lparam(membership,integer)
        llae::result<void> set_membership(std::string_view multicast_addr,
            std::optional<std::string_view> interface_addr, uv_membership membership);
        /// @luabind
        /// @lparam(multicast_addr,string)
        /// @lparam(interface_addr,string?)
        /// @lparam(source_addr,string)
        /// @lparam(membership,integer)
        llae::result<void> set_source_membership(std::string_view multicast_addr,
            std::optional<std::string_view> interface_addr, std::string_view source_addr, uv_membership membership);
        /// @luabind
        llae::result<void> set_multicast_loop(bool enable);
        /// @luabind
        llae::result<void> set_multicast_ttl(int ttl);
        /// @luabind
        llae::result<void> set_multicast_interface(std::optional<std::string_view> interface_addr);
        /// @luabind
        /// @lreturn(address,net.address?)
        /// @lreturn(error,string?)
        llae::result<net::address> getpeername() const;
        /// @luabind
        /// @lreturn(address,net.address?)
        /// @lreturn(error,string?)
        llae::result<net::address> getsockname() const;
        /// @luabind
        /// @lparam(buffer,llae.buffer)
        void add_buffer(llae::buffer_ptr&& buffer);
        /// @luabind
        llae::result<void> disconnect();
        /// Receives a single datagram: returns the payload and the source address
        /// (or `nil, error`).
        /// @luabind(async=true)
        /// @lreturn(data,llae.buffer?)
        /// @lreturn(address,net.address|string?)
        llae::result_promise_ptr<datagram> recv();
        int start_recv( const udp_recv_consumer_ptr& consumer );
        /// @luabind(name=stop_recv)
        void stop_recv();
        /// @luabind
        void close();

        /// @luabind(ltype=integer)
        static constexpr auto IPV6ONLY = UV_UDP_IPV6ONLY;
        /// @luabind(ltype=integer)
        static constexpr auto PARTIAL = UV_UDP_PARTIAL;
        /// @luabind(ltype=integer)
        static constexpr auto REUSEADDR = UV_UDP_REUSEADDR;
        /// @luabind(ltype=integer)
        static constexpr auto MMSG_CHUNK = UV_UDP_MMSG_CHUNK;
        /// @luabind(ltype=integer)
        static constexpr auto MMSG_FREE = UV_UDP_MMSG_FREE;
        /// @luabind(ltype=integer)
        static constexpr auto LINUX_RECVERR = UV_UDP_LINUX_RECVERR;
        /// @luabind(ltype=integer)
        static constexpr auto REUSEPORT = UV_UDP_REUSEPORT;
        /// @luabind(ltype=integer)
        static constexpr auto RECVMMSG = UV_UDP_RECVMMSG;
        /// @luabind(ltype=integer)
        static constexpr auto LEAVE_GROUP = UV_LEAVE_GROUP;
        /// @luabind(ltype=integer)
        static constexpr auto JOIN_GROUP = UV_JOIN_GROUP;
    };

    /// Lua wrapper for `udp:bind()`, declared here so the generated module binding
    /// can reference it.
    llae::result<void> lua_udp_bind(lua::state& l);

    /// Lua wrapper for `udp:send()`: accepts a string, a buffer or a table of them.
    llae::result_promise_ptr<void> lua_udp_send(lua::state& l);

}

namespace lua {
    /// Pushes the datagram payload and its source address.
    template <>
    struct stack<uv::datagram> {
        static int push(lua::state& l, const uv::datagram& datagram);
    };
}

#endif /*__LLAE_UV_UDP_H_INCLUDED__*/
