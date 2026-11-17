#ifndef __LLAE_NET_ADDRESS_LUA_H_INCLUDED__
#define __LLAE_NET_ADDRESS_LUA_H_INCLUDED__

#include "address.h"
#include "lua/state.h"

namespace lua {

	/// Reads a `net.address` object, or an IP address string at `idx` with an
	/// optional port at `idx + 1`. An explicit port overrides the stored one.
	template<>
	struct stack<net::address> {
		static net::address get(lua::state& s, int idx);
		static int push(lua::state& s, const net::address& v);
	};
	template<>
	struct stack<const net::address&> : stack<net::address> {};

}

#endif /*__LLAE_NET_ADDRESS_LUA_H_INCLUDED__*/
