#include "address.h"
#include "llae-private/uv.h"
#include "meta/info.h"
#include <cstring>

META_INFO(net::address,void)

namespace net {

	namespace {

		/// Zone for display: interface name on POSIX, numeric scope id on Windows.
		std::string scope_id_to_string(int scope_id) {
#ifdef _WIN32
			return std::to_string(scope_id);
#else
			char ifname[UV_IF_NAMESIZE];
			size_t ifname_len = sizeof(ifname);
			if (uv_if_indextoname(static_cast<unsigned int>(scope_id), ifname, &ifname_len) == 0) {
				return std::string(ifname, ifname_len);
			}
			return std::to_string(scope_id);
#endif
		}

	}

	address::address() : ip(), scope_id(-1), type(ip_type::none), port(0) {
		std::memset(&ip, 0, sizeof(ip));
	}

	llae::buffer_view address::get_ip() const {
		switch (type) {
		case ip_type::ipv4:
			return llae::buffer_view(ip.ipv4, sizeof(ip.ipv4));
		case ip_type::ipv6:
			return llae::buffer_view(ip.ipv6, sizeof(ip.ipv6));
		case ip_type::none:
			break;
		}
		return llae::buffer_view();
	}

	llae::result<void> address::set_ip(ip_type new_type, llae::buffer_view new_ip) {
		switch (new_type) {
		case ip_type::ipv4:
			if (new_ip.get_len() != sizeof(ip.ipv4)) {
				return llae::string_error::create("invalid IPv4 address size");
			}
			std::memset(&ip, 0, sizeof(ip));
			std::memcpy(ip.ipv4, new_ip.get_base(), sizeof(ip.ipv4));
			break;
		case ip_type::ipv6:
			if (new_ip.get_len() != sizeof(ip.ipv6)) {
				return llae::string_error::create("invalid IPv6 address size");
			}
			std::memset(&ip, 0, sizeof(ip));
			std::memcpy(ip.ipv6, new_ip.get_base(), sizeof(ip.ipv6));
			break;
		case ip_type::none:
			std::memset(&ip, 0, sizeof(ip));
			scope_id = -1;
			type = new_type;
			return llae::result<void>();
		}
		if (new_type != ip_type::ipv6) {
			scope_id = -1;
		}
		type = new_type;
		return llae::result<void>();
	}

	std::string address::get_ip_string() const {
		char buf[64];
		buf[0] = 0;
		if (type == ip_type::ipv4) {
			struct sockaddr_in addr;
			std::memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			std::memcpy(&addr.sin_addr, ip.ipv4, sizeof(ip.ipv4));
			if (uv_ip4_name(&addr, buf, sizeof(buf)) != 0) {
				return {};
			}
			return buf;
		}
		if (type == ip_type::ipv6) {
			struct sockaddr_in6 addr;
			std::memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
			std::memcpy(&addr.sin6_addr, ip.ipv6, sizeof(ip.ipv6));
			if (uv_ip6_name(&addr, buf, sizeof(buf)) != 0) {
				return {};
			}
			return buf;
		}
		return {};
	}

	std::string address::get_ip_string_with_interface() const {
		auto str = get_ip_string();
		if (type == ip_type::ipv6 && scope_id > 0) {
			str += '%';
			str += scope_id_to_string(scope_id);
		}
		return str;
	}

	llae::result<address> address::from_string(std::string_view host, uint16_t port) {
		if (host.empty()) {
			return llae::string_error::create("invalid IP address [empty]");
		}
		std::string host_str(host);
		address res;
		res.port = port;
		struct sockaddr_in addr4;
		if (uv_ip4_addr(host_str.c_str(), int(port), &addr4) == 0) {
			std::memcpy(res.ip.ipv4, &addr4.sin_addr, sizeof(res.ip.ipv4));
			res.type = ip_type::ipv4;
			return llae::result<address>(std::move(res));
		}
		struct sockaddr_in6 addr6;
		if (uv_ip6_addr(host_str.c_str(), int(port), &addr6) == 0) {
			if (host.find('%') != std::string_view::npos && addr6.sin6_scope_id == 0) {
				return llae::string_error::create("unknown IPv6 zone [" + host_str + "]");
			}
			std::memcpy(res.ip.ipv6, &addr6.sin6_addr, sizeof(res.ip.ipv6));
			if (addr6.sin6_scope_id != 0) {
				res.scope_id = static_cast<int>(addr6.sin6_scope_id);
			}
			res.type = ip_type::ipv6;
			return llae::result<address>(std::move(res));
		}
		return llae::string_error::create("invalid IP address [" + host_str + "]");
	}

	address address::from_sockaddr(const struct sockaddr* sa) {
		address res;
		if (!sa) {
			return res;
		}
		if (sa->sa_family == AF_INET) {
			auto addr4 = reinterpret_cast<const struct sockaddr_in*>(sa);
			std::memcpy(res.ip.ipv4, &addr4->sin_addr, sizeof(res.ip.ipv4));
			res.type = ip_type::ipv4;
			res.port = ntohs(addr4->sin_port);
		} else if (sa->sa_family == AF_INET6) {
			auto addr6 = reinterpret_cast<const struct sockaddr_in6*>(sa);
			std::memcpy(res.ip.ipv6, &addr6->sin6_addr, sizeof(res.ip.ipv6));
			res.type = ip_type::ipv6;
			if (addr6->sin6_scope_id != 0) {
				res.scope_id = static_cast<int>(addr6->sin6_scope_id);
			}
			res.port = ntohs(addr6->sin6_port);
		}
		return res;
	}

	llae::result<address> address::decode(const struct sockaddr* sa) {
		auto res = from_sockaddr(sa);
		if (!res.valid()) {
			return llae::string_error::create("unsupported socket address family");
		}
		return llae::result<address>(std::move(res));
	}

	llae::result<void> address::to_sockaddr(struct sockaddr_storage& out) const {
		std::memset(&out, 0, sizeof(out));
		auto ip_str = get_ip_string();
		if (ip_str.empty()) {
			return llae::string_error::create("address is not set");
		}
		if (type == ip_type::ipv4) {
			auto addr4 = reinterpret_cast<struct sockaddr_in*>(&out);
			int r = uv_ip4_addr(ip_str.c_str(), int(port), addr4);
			if (r < 0) {
				return llae::string_error::create("invalid IPv4 address");
			}
			return llae::result<void>();
		}
		if (type == ip_type::ipv6) {
			auto addr6 = reinterpret_cast<struct sockaddr_in6*>(&out);
			int r = uv_ip6_addr(ip_str.c_str(), int(port), addr6);
			if (r < 0) {
				return llae::string_error::create("invalid IPv6 address");
			}
			addr6->sin6_scope_id = scope_id > 0 ? static_cast<unsigned int>(scope_id) : 0;
			return llae::result<void>();
		}
		return llae::string_error::create("address is not set");
	}

}
