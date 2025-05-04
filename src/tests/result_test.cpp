#include "result_test.h"
#include "llae/result.h"
#include "lua/bind.h"

namespace tests {

    static llae::result<void> lua_test_err() {
        return llae::string_error::create("failed");
    }
}

int luaopen_result_tests(lua_State* L) {
	lua::state l(L);


    l.createtable();
    lua::bind::function(l,"test_err",&tests::lua_test_err);

	return 1;
}