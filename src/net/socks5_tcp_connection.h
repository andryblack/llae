#pragma once
#include "uv/tcp_connection.h"
#include "llae/promise.h"
#include <string>
#include <string_view>

namespace llae {
	class loop;
}

namespace net {

	namespace socks5 {

		class tcp_connection : public uv::tcp_connection {
			META_OBJECT
		protected:
			struct sockaddr_storage m_socks_addr;
			std::string m_socks_user;
			std::string m_socks_pass;
			llae::result_promise_ptr<void> m_connect_promise;
			class connect_req;
			struct sockaddr_storage m_connect_addr;
			void on_connected(int status);
			void report_connect_error(llae::error_ptr err);
			void report_connect_success();
			enum state_t {
				st_none,
				st_connect,
                st_select_method,
                st_username_password,
                st_open_connection,
                st_connected,
			} m_state = st_none;
            virtual void on_closed() override final;
            class write_connect_req;
            class connect_read_consumer;
            void on_connect_writed(int status);
            bool on_connect_read(ssize_t nread, llae::buffer_ptr& buffer);
            void on_connect_stop_read();
            virtual void destroy() override final;
		public:
			explicit tcp_connection(uv::loop& loop,
				const struct sockaddr_storage& addr,
				std::string&& user, std::string&& pass);
            ~tcp_connection();
			/// @luabind(name=new)
			static lua::multiret lnew(lua::state& l);
			/// @luabind(async=true)
			llae::result_promise_ptr<void> connect(llae::loop& l, std::string_view host, int port);
	  	};
    
        using tcp_connection_ptr = common::intrusive_ptr<tcp_connection>;

	}
}
