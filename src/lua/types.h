#ifndef _LLAE_LUA_TYPES_H_ICNLUDED_
#define _LLAE_LUA_TYPES_H_ICNLUDED_


extern "C" {
#include <lua.h>
}

namespace lua {

	enum class value_type {
		none = LUA_TNONE,
		boolean = LUA_TBOOLEAN,
		lightuserdata = LUA_TLIGHTUSERDATA,
		nil = LUA_TNIL,
		number = LUA_TNUMBER,
		string = LUA_TSTRING,
		table = LUA_TTABLE,
		function = LUA_TFUNCTION,
		userdata = LUA_TUSERDATA,
		thread = LUA_TTHREAD,
	};

	enum class status {
		ok = LUA_OK,
		yield = LUA_YIELD,
		errun = LUA_ERRRUN,
		errsyntax = LUA_ERRSYNTAX,
		errmem = LUA_ERRMEM,
		errgcmm = LUA_ERRGCMM,
		errerr = LUA_ERRERR,
	};

}

#endif /*_LLAE_LUA_TYPES_H_ICNLUDED_*/