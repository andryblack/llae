#include "address_lua.h"
#include "lua/metatable.h"

namespace lua {

	net::address stack<net::address>::get(lua::state& s, int idx) {
		if (s.isuserdata(idx)) {
			auto holder = meta_holder_base_t::get(s, idx);
			if (holder) {
				auto ptr = holder->get_ptr<net::address>();
				if (ptr.first && !ptr.second) {
					net::address res = *ptr.first;
					// An explicit port after the address overrides the stored one.
					if (s.isnumber(idx + 1)) {
						res.port = static_cast<uint16_t>(s.tointeger(idx + 1));
					}
					return res;
				}
			}
			s.argerror(idx, "net.address expected");
		}
		if (s.isstring(idx)) {
			int port = s.isnumber(idx + 1) ? int(s.tointeger(idx + 1)) : 0;
			auto res = net::address::from_string(s.checkstring(idx), static_cast<uint16_t>(port));
			if (!res.has_result()) {
				s.argerror(idx, res.get_error()->to_string().c_str());
			}
			return res.get_result();
		}
		s.argerror(idx, "net.address or IP address string expected");
		return net::address();
	}

	int stack<net::address>::push(lua::state& s, const net::address& v) {
		push_raw(s, v);
		return 1;
	}

}
