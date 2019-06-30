#ifndef _LUA_TYPES_H_ICNLUDED_
#define _LUA_TYPES_H_ICNLUDED_


extern "C" {
#include <lua.h>
}

namespace Lua {

	enum class ValueType {
		NONE = LUA_TNONE,
		BOOLEAN = LUA_TBOOLEAN,
		LIGHTUSERDATA = LUA_TLIGHTUSERDATA,
		NIL = LUA_TNIL,
		NUMBER = LUA_TNUMBER,
		STRING = LUA_TSTRING,
		TABLE = LUA_TTABLE,
		FUNCTION = LUA_TFUNCTION,
		USERDATA = LUA_TUSERDATA,
		THREAD = LUA_TTHREAD,
	};

	enum class Status {
		OK = LUA_OK,
		YIELD = LUA_YIELD,
		ERRRUN = LUA_ERRRUN,
		ERRSYNTAX = LUA_ERRSYNTAX,
		ERRMEM = LUA_ERRMEM,
		ERRGCMM = LUA_ERRGCMM,
		ERRERR = LUA_ERRERR,
	};

}

#endif /*_LUA_TYPES_H_ICNLUDED_*/