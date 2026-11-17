#include "udp.h"
#include "llae/app.h"
#include "llae/result.h"
#include "luv.h"
#include "net/address_lua.h"
#include "lua/stack.h"
#include "llae/logger.h"

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
                         buffers,static_cast<unsigned int>(count),addr,&udp_send_req::send_cb);
        if (r < 0) {
            remove_ref();
        }
        return r;
    }

    namespace {

        class udp_send_promise_req : public udp_send_req {
        private:
            llae::result_promise_ptr<void> m_promise;
            llae::write_buffers m_buffers;
        protected:
            virtual void on_send(int status) override {
                auto& l = llae::app::get(get_loop()).lua();
                m_buffers.reset(l);
                if (!m_promise) {
                    return;
                }
                auto promise = std::move(m_promise);
                m_promise.reset();
                if (status < 0) {
                    promise->set_result(uv::status_error::create(status));
                } else {
                    promise->set_result(llae::result<void>());
                }
            }
        public:
            udp_send_promise_req(udp_ptr&& stream,const llae::result_promise_ptr<void>& promise,
                                 llae::write_buffers&& buffers)
                : udp_send_req(std::move(stream)),m_promise(promise),m_buffers(std::move(buffers)) {
            }
            int send(const net::address* addr) {
                struct sockaddr_storage storage;
                const struct sockaddr* sa = nullptr;
                if (addr) {
                    auto res = addr->to_sockaddr(storage);
                    if (res.get_error()) {
                        return UV_EINVAL;
                    }
                    sa = reinterpret_cast<const struct sockaddr*>(&storage);
                }
                auto buffers = get_buffers(m_buffers.get_buffers());
                return start_send(buffers.data(),buffers.size(),sa);
            }
        };

        class promise_recv_consumer : public udp_recv_consumer {
        private:
            llae::result_promise_ptr<datagram> m_promise;
        public:
            explicit promise_recv_consumer(const llae::result_promise_ptr<datagram>& promise) : m_promise(promise) {
            }
            virtual bool on_recv(udp* s,ssize_t nread,llae::buffer_ptr&& buffer,
                                 const net::address& addr, unsigned /*flags*/) override final {
                auto promise = std::move(m_promise);
                m_promise.reset();
                s->stop_recv();
                if (!promise) {
                    return true;
                }
                if (nread < 0) {
                    promise->set_result(uv::status_error::create(static_cast<int>(nread)));
                    return true;
                }
                buffer->set_len(nread);
                datagram msg{std::move(buffer),addr};
                promise->set_result(llae::result<datagram>(std::move(msg)));
                return true;
            }
            virtual void on_stop_recv(udp* /*s*/) override final {
                if (!m_promise) {
                    return;
                }
                auto promise = std::move(m_promise);
                m_promise.reset();
                promise->set_result(llae::string_error::create("udp:recv stopped"));
            }
        };

    }


    udp::udp(uv::loop& loop,int af) {
        int r = uv_udp_init_ex(loop.native(),&m_udp,af);
        UV_DIAG_CHECK(r);
        attach();
    }
    udp::~udp() {
    }

    void udp::on_closed() {
        
    }

    int udp::do_bind(const net::address& address,int flags) {
        struct sockaddr_storage addr;
        auto res = address.to_sockaddr(addr);
        if (res.get_error()) {
            return UV_EINVAL;
        }
        return uv_udp_bind(&m_udp,reinterpret_cast<struct sockaddr*>(&addr),flags);
    }
    int udp::do_connect(const net::address& address) {
        struct sockaddr_storage addr;
        auto res = address.to_sockaddr(addr);
        if (res.get_error()) {
            return UV_EINVAL;
        }
        return uv_udp_connect(&m_udp,reinterpret_cast<struct sockaddr*>(&addr));
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

    llae::result<void> udp::bind(const net::address& address, std::optional<int> flags) {
        return return_status(do_bind(address,flags.value_or(0)));
    }

    llae::result<void> udp::connect(const net::address& address) {
        int r = do_connect(address);
        if (r < 0) {
            return uv::status_error::create(r);
        }
        m_connected = true;
        return llae::result<void>();
    }

    llae::result<void> udp::disconnect() {
        int r = uv_udp_connect(&m_udp,nullptr);
        if (r < 0) {
            return uv::status_error::create(r);
        }
        m_connected = false;
        return llae::result<void>();
    }

    llae::result<void> udp::set_ttl(int ttl) {
        return return_status(do_set_ttl(ttl));
    }

    llae::result<void> udp::set_broadcast(bool enable) {
        return return_status(do_set_broadcast(enable));
    }

    llae::result<void> udp::set_membership(std::string_view multicast_addr,
            std::optional<std::string_view> interface_addr, uv_membership membership) {
        std::string multicast(multicast_addr);
        std::string iface(interface_addr.value_or(std::string_view()));
        return return_status(do_set_membership(multicast.c_str(),
            interface_addr.has_value() ? iface.c_str() : nullptr,membership));
    }

    llae::result<void> udp::set_source_membership(std::string_view multicast_addr,
            std::optional<std::string_view> interface_addr,std::string_view source_addr,
            uv_membership membership) {
        std::string multicast(multicast_addr);
        std::string iface(interface_addr.value_or(std::string_view()));
        std::string source(source_addr);
        return return_status(do_set_source_membership(multicast.c_str(),
            interface_addr.has_value() ? iface.c_str() : nullptr,source.c_str(),membership));
    }

    llae::result<void> udp::set_multicast_loop(bool enable) {
        return return_status(do_set_multicast_loop(enable));
    }

    llae::result<void> udp::set_multicast_ttl(int ttl) {
        return return_status(do_set_multicast_ttl(ttl));
    }

    llae::result<void> udp::set_multicast_interface(std::optional<std::string_view> interface_addr) {
        std::string iface(interface_addr.value_or(std::string_view()));
        return return_status(do_set_multicast_interface(interface_addr.has_value() ? iface.c_str() : nullptr));
    }

    llae::result<int> udp::try_send(llae::buffer_view data, std::optional<net::address> address) {
        struct sockaddr_storage storage;
        const struct sockaddr* sa = nullptr;
        if (address) {
            if (address->port == 0) {
                return llae::string_error::create("udp::try_send port is required");
            }
            auto res = address->to_sockaddr(storage);
            if (res.get_error()) {
                return res.get_error();
            }
            sa = reinterpret_cast<const struct sockaddr*>(&storage);
        }
        uv_buf_t buf{};
        buf.base = const_cast<char*>(static_cast<const char*>(data.get_base()));
        buf.len = data.get_len();
        int r = uv_udp_try_send(get_udp(),&buf,1,sa);
        if (r < 0) {
            return uv::status_error::create(r);
        }
        return llae::result<int>(r);
    }

    llae::result_promise_ptr<void> udp::async_send(llae::loop& /*l*/, llae::write_buffers&& buffers,
            std::optional<net::address> address) {
        if (buffers.empty()) {
            auto promise = common::make_intrusive<llae::result_promise<void>>();
            promise->set_result(llae::result<void>());
            return promise;
        }
        if (address && address->port == 0) {
            return llae::make_result_promise_string_error<void>("udp::send port is required");
        }
        auto promise = common::make_intrusive<llae::result_promise<void>>();
        common::intrusive_ptr<udp_send_promise_req> req{
            new udp_send_promise_req(udp_ptr(this),promise,std::move(buffers))};
        int r = req->send(address ? &*address : nullptr);
        if (r < 0) {
            promise->set_result(uv::status_error::create(r));
        }
        return promise;
    }

    llae::result_promise_ptr<datagram> udp::recv() {
        if (m_recv_consumer) {
            return llae::make_result_promise_string_error<datagram>("udp:recv already recv");
        }
        auto promise = common::make_intrusive<llae::result_promise<datagram>>();
        common::intrusive_ptr<promise_recv_consumer> consumer{new promise_recv_consumer(promise)};
        int res = start_recv(consumer);
        if (res < 0) {
            return llae::make_result_promise_string_error<datagram>(uv::get_error(res));
        }
        return promise;
    }

    void udp::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        udp* self = static_cast<udp*>(handle->data);
        llae::buffer_ptr b;
        if (!self->m_buffers.empty()) {
            b = std::move(self->m_buffers.back());
            self->m_buffers.pop_back();
        } else {
            b = llae::buffer::alloc(suggested_size);
        }
        buf->base = static_cast<char*>(b->get_base());
        buf->len = b->get_capacity();
        b->add_ref();
    }

    void udp::add_buffer(llae::buffer_ptr&& buffer) {
        if (buffer) {
            m_buffers.emplace_back(std::move(buffer));
        } else {
            
        }
    }

    void udp::recv_cb(uv_udp_t* u, ssize_t nread, const uv_buf_t* buf,const struct sockaddr* addr, unsigned flags) {
        udp* self = static_cast<udp*>(u->data);
        auto b = llae::buffer::get(buf->base);
        auto recv_addr = net::address::from_sockaddr(addr);
        if (self->on_recv(nread,llae::buffer_ptr(b),recv_addr,flags)) {
            //std::cout << "stream read_cb remove_ref" << std::endl;
            self->remove_ref();
        }
        if (b) {
            b->remove_ref();
        }
    }

    bool udp::on_recv(ssize_t nread, llae::buffer_ptr&& buffer,const net::address& addr, unsigned flags) {
        if (m_recv_consumer) {
            auto consumer = std::move(m_recv_consumer);
            bool res = consumer->on_recv(this,
                                                nread, std::move(buffer),addr,flags);
            if (!res) {
                m_recv_consumer = std::move(consumer);
            }
            return res;
        } else {
            LOG_ERROR("recv without consumer" );
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

    namespace {

        llae::result<net::address> udp_socket_address(const uv_udp_t* handle, bool peer) {
            struct sockaddr_storage addr;
            int namelen = sizeof(addr);
            auto h = const_cast<uv_udp_t*>(handle);
            auto r = peer ? uv_udp_getpeername(h,reinterpret_cast<struct sockaddr*>(&addr),&namelen)
                          : uv_udp_getsockname(h,reinterpret_cast<struct sockaddr*>(&addr),&namelen);
            if (r < 0) {
                return uv::status_error::create(r);
            }
            return net::address::decode(reinterpret_cast<struct sockaddr*>(&addr));
        }

    }

    llae::result<net::address> udp::getpeername() const {
        return udp_socket_address(&m_udp, true);
    }

    llae::result<net::address> udp::getsockname() const {
        return udp_socket_address(&m_udp, false);
    }

    void udp::stop_recv() {
        uv_udp_recv_stop(get_udp());
        if (m_recv_consumer) {
            m_recv_consumer->on_stop_recv(this);
        }
        m_recv_consumer.reset();
    }

    void udp::close() {
        if (is_closing()) {
            return;
        }
        stop_recv();
        handle::close();
    }

    llae::result<void> lua_udp_bind(lua::state& l) {
        auto self = lua::stack<udp_ptr>::get(l,1);
        if (!self) {
            l.argerror(1,"need udp");
        }
        auto address = lua::stack<net::address>::get(l,2);
        auto flags = l.optinteger(l.isuserdata(2) ? 3 : 4,0);
        return self->bind(address,static_cast<int>(flags));
    }

    llae::result_promise_ptr<void> lua_udp_send(lua::state& l) {
        auto self = lua::stack<udp_ptr>::get(l,1);
        if (!self) {
            l.argerror(1,"need udp");
        }
        std::optional<net::address> address;
        if (l.gettop() > 2) {
            address = lua::stack<net::address>::get(l,3);
            if (address->port == 0) {
                return llae::make_result_promise_string_error<void>("udp::send port is required");
            }
        }
        llae::write_buffers buffers;
        l.pushvalue(2);
        if (!buffers.put(l)) {
            buffers.reset(l);
            return llae::make_result_promise_string_error<void>("udp::send invalid data");
        }
        return self->async_send(llae::loop::get(l),std::move(buffers),address);
    }

    lua::multiret udp::lnew(lua::state& l) {
        auto af = l.optinteger(1,AF_UNSPEC);
        common::intrusive_ptr<udp> server{new udp(llae::app::get(l).loop(),af)};
        lua::push(l,std::move(server));
        return {1};
    }
}

namespace lua {

    int stack<uv::datagram>::push(lua::state& l, const uv::datagram& datagram) {
        int res = lua::push(l,datagram.data);
        res += lua::push(l,datagram.address);
        return res;
    }

}
