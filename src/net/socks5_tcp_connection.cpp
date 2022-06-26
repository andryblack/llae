#include "socks5_tcp_connection.h"
#include "llae/app.h"
#include "uv/luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include <iostream>

META_OBJECT_INFO(net::socks5::tcp_connection,uv::tcp_connection)


namespace net { namespace socks5 {


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
            add_ref();
            int r = uv_tcp_connect(&m_req,m_conn->get_tcp(),
                                   (struct sockaddr *)&m_conn->m_socks_addr,&connect_req::connect_cb);
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
        virtual void on_write(int status) {
            m_conn->on_connect_writed(status);
        }
    };

    class tcp_connection::connect_read_consumer : public uv::stream_read_consumer {
    private:
        tcp_connection_ptr m_conn;
    public:
        explicit connect_read_consumer(tcp_connection_ptr&& con) : m_conn(std::move(con)) {}
        virtual bool on_read(uv::stream* s,ssize_t nread, const uv::buffer_ptr&& buffer) override {
            if (m_conn) {
                return m_conn->on_connect_read(nread,std::move(buffer));
            }
            return true;
        }
        virtual void on_stream_closed(stream* s) override {
            if (m_conn) {
                m_conn->on_connect_stream_closed();
                m_conn.reset();
            }
        }
        virtual void on_stop_read(stream* s) override {
            if (m_conn) {
                m_conn->on_connect_stop_read();
                m_conn.reset();
            }
        }
    };

	tcp_connection::tcp_connection(uv::loop& loop,const struct sockaddr_storage& addr,
				std::string&& user, std::string&& pass) : uv::tcp_connection(loop),
				m_socks_addr(addr),m_socks_user(std::move(user)),
				m_socks_pass(std::move(pass)) {
	}
	tcp_connection::~tcp_connection() {
 	}

    void tcp_connection::on_closed() {
        if (llae::app::closed(get_stream()->loop)) {
            m_connect_cont.release();
        } else {
            m_connect_cont.reset(llae::app::get(get_stream()->loop).lua());
        }
        uv::tcp_connection::on_closed();
    }

	lua::multiret tcp_connection::lnew(lua::state& l) {
		
		const char* host = l.checkstring(1);
		int port = l.checkinteger(2);
		struct sockaddr_storage addr;
		if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
	      	uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
	    	l.error("invalid sock IP address or port [%s:%d]", host, port);
	   	}
	   	std::string name(l.optstring(3,""));
	   	std::string pass(l.optstring(4,""));

		common::intrusive_ptr<tcp_connection> connection{new tcp_connection(llae::app::get(l).loop(),addr,
			std::move(name),std::move(pass))};
		lua::push(l,std::move(connection));
		return {1};
	}

    static inline void push_uv_error(lua::state& l,int status) {
        l.pushstring("SOCKS5: ");
        uv::push_error(l, status);
        l.concat(2);
    }

	template <typename Report>
	void tcp_connection::report_connect_error(Report report) {
        m_state = st_none;
        stop_read();
		if (llae::app::closed(get_handle()->loop)) {
            m_connect_cont.release();
            return;
        }
		auto& l = llae::app::get(get_handle()->loop).lua();
        if (!l.native()) {
            m_connect_cont.release();
            return;
        }
        l.checkstack(2);
		m_connect_cont.push(l);
		m_connect_cont.reset(l); // on stack
		auto toth = l.tothread(-1);
		toth.checkstack(3);
		toth.pushnil();
		report(toth);
		auto s = toth.resume(l,2);
		if (s != lua::status::ok && s != lua::status::yield) {
			llae::app::show_error(toth,s);
		}
		l.pop(1);// thread
	}

    void tcp_connection::on_connect_stop_read() {
        if (!m_connect_cont.valid() || m_state==st_none) {
            return; // wtf?
        }
    }

    void tcp_connection::on_connect_stream_closed() {
        if (!m_connect_cont.valid() || m_state==st_none) {
            return; // wtf?
        }
        report_connect_error([](lua::state&l){
            l.pushstring("SOCKS5: connection closed");
        });
    }

    bool tcp_connection::on_connect_read(ssize_t nread, const uv::buffer_ptr&& buffer) {
        if (!m_connect_cont.valid() || m_state==st_none) {
            return true; // wtf?
        }
        if (st_select_method == m_state) {
            if (nread < 2) {
                report_connect_error([](lua::state&l){
                    l.pushstring("SOCKS5: auth failed");
                });
                return true;
            }
            auto data = static_cast<const uint8_t*>(buffer->get_base());
            if (data[0]!=0x05 || data[1]==0xff) {
                report_connect_error([](lua::state&l){
                    l.pushstring("SOCKS5: auth unsupported");
                });
                return true;
            }
            if (m_socks_user.empty()) {
                if (data[1]!=0x00) {
                    report_connect_error([](lua::state&l){
                        l.pushstring("SOCKS5: invalid auth selection");
                    });
                    return true;
                }
            } else {
                if (data[1]!=0x02) {
                    report_connect_error([](lua::state&l){
                        l.pushstring("SOCKS5: invalid auth selection");
                    });
                    return true;
                }
                m_state = st_username_password;
                common::intrusive_ptr<write_connect_req> req(new write_connect_req(tcp_connection_ptr(this)));
                req->write(0x01);
                req->write(m_socks_user.length());
                req->write(m_socks_user);
                req->write(m_socks_pass.length());
                req->write(m_socks_pass);
                int status = req->start();
                if (status < 0) {
                    report_connect_error([status](lua::state& l) {
                        push_uv_error(l,status);
                    });
                    return true;
                }
                return false;
            }
        } else if (st_username_password == m_state) {
            if (nread < 2) {
                report_connect_error([](lua::state&l){
                    l.pushstring("SOCKS5: auth failed");
                });
                return true;
            }
            auto data = static_cast<const uint8_t*>(buffer->get_base());
            if (data[0]!=0x01 || data[1]!=0x00) {
                report_connect_error([](lua::state&l){
                    l.pushstring("SOCKS5: auth failed name pass");
                });
                return true;
            }
            m_state = st_open_connection;
            common::intrusive_ptr<write_connect_req> req(new write_connect_req(tcp_connection_ptr(this)));
            req->write(0x05); // VER
            req->write(0x01); // CMD
            req->write(0x00); // RSV
            auto addr = (const struct sockaddr *)&m_connect_addr;
            
            if (addr->sa_family == AF_INET) {
                auto addr4 = (const struct sockaddr_in *)&m_connect_addr;
                req->write(0x01); // ATYP
                req->write(&addr4->sin_addr,4);
                req->write(addr4->sin_port);
                req->write(addr4->sin_port >> 8);
            } else if (addr->sa_family == AF_INET6) {
                auto addr6 = (const struct sockaddr_in6 *)&m_connect_addr;
                req->write(0x04); // ATYP
                req->write(&addr6->sin6_addr,16);
                req->write(addr6->sin6_port);
                req->write(addr6->sin6_port >> 8);
            }
            
            int status = req->start();
            if (status < 0) {
                report_connect_error([status](lua::state& l) {
                    push_uv_error(l,status);
                });
                return true;
            }
            return false;
        } else if (st_open_connection == m_state) {
            if (nread < 4) {
                report_connect_error([](lua::state&l){
                    l.pushstring("SOCKS5: auth failed");
                });
                return true;
            }
            auto data = static_cast<const uint8_t*>(buffer->get_base());
            if (data[0]!=0x05 || data[1]!=0x00) {
                report_connect_error([](lua::state&l){
                    l.pushstring("SOCKS5: connect failed");
                });
                return true;
            }
            stop_read();
            if (m_connect_cont.valid()) {
                m_state = st_connected;
                auto& l = llae::app::get(get_handle()->loop).lua();
                if (!l.native()) {
                    m_connect_cont.release();
                    return true;
                }
                l.checkstack(2);
                m_connect_cont.push(l);
                m_connect_cont.reset(l); // on stack
                auto toth = l.tothread(-1);
                toth.checkstack(3);
                toth.pushboolean(true);
                auto s = toth.resume(l,1);
                if (s != lua::status::ok && s != lua::status::yield) {
                    llae::app::show_error(toth,s);
                }
                l.pop(1);// thread
            }
            return true;
        }
        return true;
    }

    void tcp_connection::on_connect_writed(int status) {
        if (!m_connect_cont.valid() || m_state==st_none) {
            return; // wtf?
        }
        if (status < 0) {
            report_connect_error([status](lua::state& l) {
                push_uv_error(l,status);
            });
        }
    }

	void tcp_connection::on_connected(int status) {
		if (!m_connect_cont.valid() || m_state!=st_connect) {
			return; // wtf?
		}
		if (status < 0) {
			report_connect_error([status](lua::state& l) {
                push_uv_error(l,status);
			});
		} else {
            int status = start_read(uv::stream_read_consumer_ptr(new connect_read_consumer(tcp_connection_ptr(this))));
            if (status < 0) {
                report_connect_error([status](lua::state& l) {
                    push_uv_error(l,status);
                });
                return;
            }
			//m_state = ;
            common::intrusive_ptr<write_connect_req> req(new write_connect_req(tcp_connection_ptr(this)));
            req->write(0x05);
            req->write(0x01);
            req->write(m_socks_user.empty() ? 0x00 : 0x02);
            m_state = st_select_method;
            status = req->start();
            if (status < 0) {
                report_connect_error([status](lua::state& l) {
                    push_uv_error(l,status);
                });
            }
		}
	}

	lua::multiret tcp_connection::connect(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("tcp_connection::connect is async");
			return {2};
		}

		if (m_state != st_none) {
			l.pushnil();
			l.pushstring("tcp_connection::connect invalid state");
			return {2};
		}

		if (m_connect_cont.valid()) {
			l.pushnil();
			l.pushstring("tcp_connection::connect already at connect");
			return {2};
		}
		
		{
			const char* host = l.checkstring(2);
			int port = l.checkinteger(3);
			
			if (uv_ip4_addr(host, port, (struct sockaddr_in*)&m_connect_addr) &&
		      	uv_ip6_addr(host, port, (struct sockaddr_in6*)&m_connect_addr)) {
		    	l.error("invalid IP address or port [%s:%d]", host, port);
		   	}

			l.pushthread();
			m_connect_cont.set(l);
			m_state = st_connect;
			common::intrusive_ptr<connect_req> req{new connect_req(tcp_connection_ptr(this))};
            int r = req->connect();
			if (r < 0) {
				m_state = st_none;
                m_connect_cont.reset(l);
				l.pushnil();
                push_uv_error(l,r);
				return {2};
			}
		} 
		l.yield(0);
		return {0};
	}

 

	void tcp_connection::lbind(lua::state& l) {
		lua::bind::function(l,"new",&tcp_connection::lnew);
		lua::bind::function(l,"connect",&tcp_connection::connect);
	}

}}
