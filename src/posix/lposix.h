#ifndef __LLAE_POSIX_LPOSIX_H_INCLUDED__
#define __LLAE_POSIX_LPOSIX_H_INCLUDED__

#include "lua/state.h"


namespace posix {

	void push_error_errno(lua::state& l);

#define BIND_M(M) l.pushinteger(M);l.setfield(-2,#M);
}

#endif /*__LLAE_POSIX_LPOSIX_H_INCLUDED__*/