#ifndef _LLAE_LUA_EMBEDDED_H_INCLUDED_
#define _LLAE_LUA_EMBEDDED_H_INCLUDED_

#include "state.h"

namespace lua {

	struct embedded_script {
		const char* name;
		const unsigned char* content;
		size_t size;
		static const embedded_script scripts[];
	};

	void attach_embedded_scripts(lua::state& lua);
	

}

#endif /*_LLAE_LUA_EMBEDDED_H_INCLUDED_*/