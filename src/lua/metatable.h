#ifndef _LLAE_LUA_METATABLE_H_INCLUDED_
#define _LLAE_LUA_METATABLE_H_INCLUDED_

#include "state.h"
#include "meta/info.h"

namespace lua {

	void create_metatable(state& s,const meta::info_t* info);
	void set_metatable(state& s,const meta::info_t* info);
	void get_metatable(state& s,const meta::info_t* info);

}

#endif /*_LLAE_LUA_METATABLE_H_INCLUDED_*/ 