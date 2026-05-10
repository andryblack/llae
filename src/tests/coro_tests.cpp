#include "coro_tests.h"
#include "common/intrusive_ptr.h"
#include "llae/buffer.h"
#include "llae/coro.h"
#include "crypto/md.h"
#include "llae/promise.h"
#include "llae/result.h"
#include "lua/bind.h"
#include "lua/state.h"
#include "llae/loop.h"
#include <cassert>


namespace tests {

    static llae::result_promise_ptr<llae::buffer_base_ptr> test_md(llae::loop& a) {
        auto md = crypto::md::create(MBEDTLS_MD_MD5);
        if (!md) {
            co_return llae::string_error::create("failed create md");
        }
        static const char* test = "test";
        auto buf = llae::buffer::hold(test,strlen(test));
        auto ru = co_await md->async_update(a, std::move(buf));
        if (ru.get_error()) {
            co_return ru.get_error();
        }
        auto fu = co_await md->async_finish(a);
        co_return fu;
    }

    static llae::result_promise_ptr<llae::buffer_base_ptr> lua_test_md(lua::state& l) {
        return test_md(llae::loop::get(l));
    }
}


int luaopen_coro_tests(lua_State* L) {
	lua::state l(L);


    l.createtable();
    lua::bind::function(l,"test_md",&tests::lua_test_md);

	return 1;
}