#pragma once

#include "headers.h"

namespace lua::debug {

    void set_main_state(lua_State* L);
    void set_current_state(lua_State* L);
	void print_stack();

}