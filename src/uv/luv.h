#ifndef __LLAE_UV_LUV_H_INCLUDED__
#define __LLAE_UV_LUV_H_INCLUDED__

#include "lua/state.h"
#include "decl.h"

namespace uv {

	void error(lua::state& l,int e);
	void error(int e,const char* file,int line);
	static inline void check_error(lua::state& l,int e) {
		if (e<0) error(l,e);
	}
#define UV_DIAG_CHECK(e) do{ if (e<0) ::uv::error(e,__FILE__,__LINE__); } while(false)
	void push_error(lua::state& s,int r);
	void print_error(int r);
}

#endif /*__LLAE_UV_LUV_H_INCLUDED__*/
