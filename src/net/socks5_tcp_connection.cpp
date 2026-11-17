#include "socks5_tcp_connection.h"
#include "llae/app.h"
#include "llae/async_bind.h"
#include "uv/luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include "address_lua.h"
#include "llae/app.h"
#include <cstdio>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

META_OBJECT_INFO(net::socks5::tcp_connection,uv::tcp_connection)


namespace net { namespace socks5 {

namespace {

	llae::error_ptr socks5_msg(std::string_view msg) {
		return llae::string_error::create(msg);
	}

	llae::error_ptr socks5_uv_error(int status) {
		return llae::string_error::create(std::string("SOCKS5: ") + uv::get_error(status));
	}

	llae::error_ptr socks5_read_error(std::string_view msg, ssize_t nread) {
		if (nread < 0) {
			return socks5_uv_error(static_cast<int>(nread));
		}
		return socks5_msg(msg);
	}

}

	class tcp_connection::connect_req : public uv::req {
	private:
		uv_connect_t m_req;
		tcp_connection_ptr m_conn;
	public:
		connect_req(tcp_connection_ptr&& con) : m_conn(std::move(con)) {
			attach(reinterpret_cast<uv_req_t*>(get()));
		}
		uv_connect_t* get() { return &m_req; }
		static void connect_cb(uv_connect_t* req, int status) {
			auto self = static_cast<connect_req*>(req->data);
			self->on_end(status);
			self->remove_ref();
		}
        int connect() {
            struct sockaddr_storage addr;
            auto res = m_conn->m_socks_addr.to_sockaddr(addr);
            if (res.get_error()) {
                return UV_EINVAL;
            }
            add_ref();
            int r = uv_tcp_connect(&m_req,m_conn->get_tcp(),
                                   (struct sockaddr *)&addr,&connect_req::connect_cb);
            if (r < 0) {
                remove_ref();
            }
            return r;
        }
		void on_end(int status) {
			if (m_conn) {
				m_conn->on_connected(status);
			}
		}
	};

    class tcp_connection::write_connect_req : public uv::write_req {
    private:
        tcp_connection_ptr m_conn;
        uint8_t m_data[512];
        uv_buf_t m_buf;
    public:
        explicit write_connect_req(tcp_connection_ptr&& self) : uv::write_req(uv::stream_ptr(self.get())),m_conn(std::move(self)) {
            m_buf.len = 0;
            m_buf.base = (char*)m_data;
        }
        void write(uint8_t d) {
            m_data[m_buf.len++] = d;
        }
        void write(const std::string& d) {
            auto l = d.length();
            memcpy(&m_data[m_buf.len], d.c_str(), l);
            m_buf.len += l;
        }
        void write(const void* d,size_t s) {
            memcpy(&m_data[m_buf.len], d, s);
            m_buf.len += s;
        }
        int start() {
            return start_write(&m_buf,1);
        }
        virtual void on_write(int status) override {
            m_conn->on_connect_writed(status);
        }
    };

    class tcp_connection::connect_read_consumer : public uv::stream_read_consumer {
    private:
        tcp_connection_ptr m_conn;
    public:
        explicit connect_read_consumer(tcp_connection_ptr&& con) : m_conn(std::move(con)) {}
        virtual bool on_read(uv::readable_stream* s,ssize_t nread, llae::buffer_ptr& buffer) override {
            if (m_conn) {
                return m_conn->on_connect_read(nread,buffer);
            }
            return true;
        }
        virtual void on_stop_read(uv::readable_stream* s) override {
            if (m_conn) {
                m_conn->on_connect_stop_read();
                m_conn.reset();
            }
        }
    };

	tcp_connection::tcp_connection(uv::loop& loop,const net::address& addr,
				std::string&& user, std::string&& pass) : uv::tcp_connection(loop),
				m_socks_addr(addr),m_socks_user(std::move(user)),
				m_socks_pass(std::move(pass)) {
	}
	tcp_connection::~tcp_connection() {
 	}

    void tcp_connection::destroy() {
        uv::tcp_connection::destroy();
    }

    void tcp_connection::on_closed() {
        if (m_connect_promise) {
			report_connect_error(socks5_msg("SOCKS5: closed"));
        }
        uv::tcp_connection::on_closed();
    }

	lua::multiret tcp_connection::lnew(lua::state& l) {
		auto addr = lua::stack<net::address>::get(l, 1);
		bool address_object = l.isuserdata(1);
		int user_idx = address_object ? 2 : 3;
		int pass_idx = address_object ? 3 : 4;
		std::string name(l.optstring(user_idx,""));
		std::string pass(l.optstring(pass_idx,""));
		// RFC 1929: ULEN and PLEN are one octet each, and the whole auth
		// request (01 ULEN USER PLEN PASS) must fit the request buffer
		if (name.length() > 255) {
			l.argerror(user_idx, "socks5 user name too long");
		}
		if (pass.length() > 255) {
			l.argerror(pass_idx, "socks5 password too long");
		}
		if (name.length() + pass.length() > 509) {
			l.argerror(user_idx, "socks5 user name and password are too long");
		}

		common::intrusive_ptr<tcp_connection> connection{new tcp_connection(llae::app::get(l).loop(),addr,
			std::move(name),std::move(pass))};
		lua::push(l,std::move(connection));
		return {1};
	}

	void tcp_connection::report_connect_error(llae::error_ptr err) {
        m_state = st_none;
        stop_read();
		if (!m_connect_promise) {
			return;
		}
		auto promise = std::move(m_connect_promise);
		m_connect_promise.reset();
		promise->set_result(llae::result<void>(std::move(err)));
	}

	void tcp_connection::report_connect_success() {
		if (!m_connect_promise) {
			return;
		}
		stop_read();
		m_state = st_connected;
		auto promise = std::move(m_connect_promise);
		m_connect_promise.reset();
		promise->set_result(llae::result<void>());
	}

    void tcp_connection::on_connect_stop_read() {
        
    }
    bool tcp_connection::on_connect_read(ssize_t nread, llae::buffer_ptr& buffer) {
        if (!m_connect_promise || m_state==st_none) {
            return true;
        }
        if (st_select_method == m_state) {
            if (nread < 2) {
                report_connect_error(socks5_read_error("SOCKS5: auth failed", nread));
                return true;
            }
            auto data = static_cast<const uint8_t*>(buffer->get_base());
            if (data[0]!=0x05 || data[1]==0xff) {
                report_connect_error(socks5_msg("SOCKS5: auth unsupported"));
                return true;
            }
            if (m_socks_user.empty()) {
                if (data[1]!=0x00) {
                    report_connect_error(socks5_msg("SOCKS5: invalid auth selection"));
                    return true;
                }
                send_connect_request();
                return false;
            } else {
                if (data[1]!=0x02) {
                    report_connect_error(socks5_msg("SOCKS5: invalid auth selection"));
                    return true;
                }
                m_state = st_username_password;
                common::intrusive_ptr<write_connect_req> req(new write_connect_req(tcp_connection_ptr(this)));
                req->write(0x01);
                req->write(static_cast<uint8_t>(m_socks_user.length()));
                req->write(m_socks_user);
                req->write(static_cast<uint8_t>(m_socks_pass.length()));
                req->write(m_socks_pass);
                int status = req->start();
                if (status < 0) {
                    report_connect_error(socks5_uv_error(status));
                    return true;
                }
                return false;
            }
        } else if (st_username_password == m_state) {
            if (nread < 2) {
                report_connect_error(socks5_read_error("SOCKS5: auth failed", nread));
                return true;
            }
            auto data = static_cast<const uint8_t*>(buffer->get_base());
            if (data[0]!=0x01 || data[1]!=0x00) {
                report_connect_error(socks5_msg("SOCKS5: auth failed name pass"));
                return true;
            }
            send_connect_request();
            return false;
        } else if (st_open_connection == m_state) {
            if (nread < 4) {
                report_connect_error(socks5_read_error("SOCKS5: auth failed", nread));
                return true;
            }
            auto data = static_cast<const uint8_t*>(buffer->get_base());
            if (data[0]!=0x05 || data[1]!=0x00) {
                report_connect_error(socks5_msg("SOCKS5: connect failed"));
                return true;
            }
            report_connect_success();
            return true;
        }
        return true;
    }

    void tcp_connection::send_connect_request() {
        m_state = st_open_connection;
        common::intrusive_ptr<write_connect_req> req(new write_connect_req(tcp_connection_ptr(this)));
        req->write(0x05); // VER
        req->write(0x01); // CMD
        req->write(0x00); // RSV
        if (!m_connect_domain.empty()) {
            if (m_connect_domain.length() > 255) {
                report_connect_error(socks5_msg("SOCKS5: host name too long"));
                return;
            }
            req->write(0x03); // ATYP: domain name, resolved by the proxy
            req->write(static_cast<uint8_t>(m_connect_domain.length()));
            req->write(m_connect_domain.data(),m_connect_domain.length());
        } else {
            switch (m_connect_addr.type) {
            case net::address::ip_type::ipv4:
                req->write(0x01); // ATYP
                req->write(m_connect_addr.ip.ipv4,4);
                break;
            case net::address::ip_type::ipv6:
                req->write(0x04); // ATYP
                req->write(m_connect_addr.ip.ipv6,16);
                break;
            default:
                report_connect_error(socks5_msg("SOCKS5: invalid addr"));
                return;
            }
        }
        req->write(static_cast<uint8_t>(m_connect_addr.port >> 8));
        req->write(static_cast<uint8_t>(m_connect_addr.port & 0xff));

        int status = req->start();
        if (status < 0) {
            report_connect_error(socks5_uv_error(status));
        }
    }

    void tcp_connection::on_connect_writed(int status) {
        if (!m_connect_promise || m_state==st_none) {
            return;
        }
        if (status < 0) {
            report_connect_error(socks5_uv_error(status));
        }
    }

	void tcp_connection::on_connected(int status) {
		if (!m_connect_promise || m_state!=st_connect) {
			return;
		}
		if (status < 0) {
			report_connect_error(socks5_uv_error(status));
		} else {
            int status = start_read(uv::stream_read_consumer_ptr(new connect_read_consumer(tcp_connection_ptr(this))));
            if (status < 0) {
                report_connect_error(socks5_uv_error(status));
                return;
            }
            common::intrusive_ptr<write_connect_req> req(new write_connect_req(tcp_connection_ptr(this)));
            req->write(0x05);
            req->write(0x01);
            req->write(m_socks_user.empty() ? 0x00 : 0x02);
            m_state = st_select_method;
            status = req->start();
            if (status < 0) {
                report_connect_error(socks5_uv_error(status));
            }
		}
	}

	llae::result_promise_ptr<void> tcp_connection::connect_address(llae::loop& /*l*/, const net::address& address) {
		return start_connect(address, std::string_view());
	}

	llae::result_promise_ptr<void> tcp_connection::connect(llae::loop& l, std::string_view host, int port) {
		auto address = net::address::from_string(host, port);
		if (address.has_result()) {
			return start_connect(address.get_result(), std::string_view());
		}
		// A domain name is sent as ATYP=0x03 with one octet length.
		if (host.size() > 255) {
			return llae::make_result_promise_string_error<void>("SOCKS5: host name too long");
		}
		net::address target;
		target.port = static_cast<uint16_t>(port);
		return start_connect(target, host);
	}

	llae::result_promise_ptr<void> tcp_connection::start_connect(const net::address& target, std::string_view domain) {
        if (is_closed() || is_closing()) {
            return llae::make_result_promise_string_error<void>("tcp_connection::connect is closed");
        }

		if (m_state != st_none) {
			return llae::make_result_promise_string_error<void>("tcp_connection::connect invalid state");
		}

		if (m_connect_promise) {
			return llae::make_result_promise_string_error<void>("tcp_connection::connect already at connect");
		}

		m_connect_addr = target;
		m_connect_domain = domain;

		auto promise = common::make_intrusive<llae::result_promise<void>>();
		m_connect_promise = promise;
		m_state = st_connect;
		common::intrusive_ptr<connect_req> req{new connect_req(tcp_connection_ptr(this))};
		int r = req->connect();
		if (r < 0) {
			m_state = st_none;
			m_connect_promise.reset();
			promise->set_result(socks5_uv_error(r));
		}
		return promise;
	}

}}
