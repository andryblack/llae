#include "common/inplace_function.h"
#include "lua/bind.h"

#include <cassert>
#include <cstddef>



namespace tests {

    struct count_t {
        static size_t c;
        count_t() {
            ++c;
        }
        count_t(count_t&&) {
            ++c;
        }
        count_t(const count_t&) = delete;
        ~count_t() {
            --c;
        }
    };

    size_t count_t::c = 0;

    static void test_inplace_function() {
        using func_t = common::inplace_function<void(int)>;

        {
            auto f1 = func_t();
            assert(!f1);
            auto f2 = func_t(nullptr);
            assert(!f2);
            auto f3 = func_t([](int){});
            assert(f3);
        }
        {
            auto f1 = func_t([c = count_t{}](int){});
            assert(count_t::c == 1);
            auto f2 = std::move(f1);
            assert(count_t::c == 1);
            {
                auto f3 = std::move(f2);
                (void)f3;
            }
            assert(count_t::c == 0);
        }
        {
            auto f = common::inplace_function<void(const count_t&)>([](const count_t&){
                assert(count_t::c == 1);
            });
            f(count_t{});
            assert(count_t::c == 0);
        }
        {
            auto f = common::inplace_function<void(count_t&&)>([](count_t&&){
                assert(count_t::c == 1);
            });
            f(count_t{});
            assert(count_t::c == 0);
        }
        {
            int x = 0;
            void* ptr1 = &x;
            void* ptr2 = ptr1;
            void* ptr3 = ptr2;
            auto f = common::inplace_function<void(void)>([&ptr1,&ptr2,&ptr3](){
                assert(ptr1);
                assert(ptr2);
                assert(ptr3);
            });
        }
    }
}


int luaopen_inplace_function_tests(lua_State* L) {
	lua::state l(L);


    l.createtable();
    lua::bind::function(l,"test",&tests::test_inplace_function);

	return 1;
}