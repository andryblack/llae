#ifndef __LLAE_UV_LUV_H_INCLUDED__
#define __LLAE_UV_LUV_H_INCLUDED__

#include "lua/state.h"
#include "decl.h"

namespace uv {

	void error(lua::state& l,int e);
	static inline void check_error(lua::state& l,int e) {
		if (e<0) error(l,e);
	}
	void push_error(lua::state& s,int r);

}

#endif /*__LLAE_UV_LUV_H_INCLUDED__*/