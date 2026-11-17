#ifndef __LLAE_NET_ADDRESS_H_INCLUDED__
#define __LLAE_NET_ADDRESS_H_INCLUDED__

#include "llae/buffer.h"
#include "llae/error.h"
#include "llae/result.h"
#include <cstdint>
#include <string>
#include <string_view>

struct sockaddr;
struct sockaddr_storage;

namespace net {

	/**
	 * Raw IPv4 or IPv6 address bytes. Only the member matching
	 * `net::address::type` is meaningful.
	 */
	union ip_data {
		uint8_t ipv4[4];
		uint8_t ipv6[16];
	};

	/**
	 * Universal network address: IPv4 or IPv6 (with an optional zone for
	 * link-local IPv6) plus a port.
	 */
	/// @luabind
	/// @luabind_include("address_lua.h")
	struct address {
		/// @luabind
		enum class ip_type {
			none,
			ipv4,
			ipv6
		};

		/// Raw constructor: Lua `net.address.new()`.
		/// @luabind(raw=true)
		address();

		ip_data ip;
		/// IPv6 zone (sin6_scope_id), -1 when not set.
		/// @luabind
		int scope_id = -1;
		/// @luabind
		ip_type type = ip_type::none;
		/// @luabind
		uint16_t port = 0;

		bool valid() const { return type != ip_type::none; }

		/// Returns the raw address bytes: 4 for IPv4, 16 for IPv6, empty for none.
		/// @luabind
		/// @lreturn(ip,string)
		llae::buffer_view get_ip() const;

		/// Sets the address bytes from a 4- or 16-byte buffer.
		/// @luabind
		/// @lparam(type,net.address.ip_type)
		/// @lparam(ip,string|llae.buffer_base)
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		llae::result<void> set_ip(ip_type type, llae::buffer_view ip);

		/// Textual address without the IPv6 zone.
		/// @luabind
		/// @lreturn(ip,string)
		std::string get_ip_string() const;

		/// Textual address with the IPv6 zone, e.g. `fe80::1%en0`.
		/// @luabind
		/// @lreturn(ip,string)
		std::string get_ip_string_with_interface() const;

		/// Parses a numeric IP address, optionally with a `%zone` suffix.
		/// @luabind
		/// @lparam(host,string)
		/// @lparam(port,integer)
		/// @lreturn(address,net.address?)
		/// @lreturn(error,string?)
		static llae::result<address> from_string(std::string_view host, uint16_t port);

		/// Decodes a socket address. Unsupported families produce an empty
		/// address (`type == ip_type::none`).
		static address from_sockaddr(const struct sockaddr* sa);

		/// Decodes a socket address, failing on an unsupported family.
		static llae::result<address> decode(const struct sockaddr* sa);

		/// Fills a socket address, ready to be passed to libuv.
		llae::result<void> to_sockaddr(struct sockaddr_storage& out) const;
	};

}

#endif /*__LLAE_NET_ADDRESS_H_INCLUDED__*/
