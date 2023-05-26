#pragma once
#include "uv/tcp_connection.h"
#include <string>

namespace net {

	namespace socks5 {

		class tcp_connection : public uv::tcp_connection {
			META_OBJECT
		protected:
			struct sockaddr_storage m_socks_addr;
			std::string m_socks_user;
			std::string m_socks_pass;
			lua::ref m_connect_cont;
			class connect_req;
			struct sockaddr_storage m_connect_addr;
			void on_connected(int status);
			template <typename Report>
			void report_connect_error(Report r);
			enum state_t {
				st_none,
				st_connect,
                st_select_method,
                st_username_password,
                st_open_connection,
                st_connected,
			} m_state = st_none;
            virtual void on_closed() override;
            class write_connect_req;
            class connect_read_consumer;
            void on_connect_writed(int status);
            void on_connect_stream_closed();
            bool on_connect_read(ssize_t nread, const uv::buffer_ptr&& buffer);
		public:
			explicit tcp_connection(uv::loop& loop,
				const struct sockaddr_storage& addr,
				std::string&& user, std::string&& pass);
            ~tcp_connection();
			static lua::multiret lnew(lua::state& l);
			lua::multiret connect(lua::state& l);
	      	static void lbind(lua::state& l);
		};
    
        using tcp_connection_ptr = common::intrusive_ptr<tcp_connection>;

	}
}
