#include "bind_tests.h"
#include <lua/bind.h>

META_INFO(tests::test_bind_fields,void)

namespace tests {
    size_t test_bind_fields::count = 0;
    test_bind_fields::test_bind_fields() {
        count++;
    }
    test_bind_fields::~test_bind_fields() {
        count--;
    }
    size_t test_bind_fields::get_count() {
        return count;
    }
    void test_bind_fields::bind(lua::state& l) {
        lua::bind::raw_constructor<test_bind_fields>(l);
        lua::bind::field(l,"field1",&test_bind_fields::field1);
        lua::bind::field(l,"field2",&test_bind_fields::field2);
        lua::bind::field(l,"field3",&test_bind_fields::field3);
        lua::bind::field_ro(l,"const_field",&test_bind_fields::const_field);
        lua::bind::function(l,"method1",&test_bind_fields::method1);
        lua::bind::function(l,"get_const",&test_bind_fields::get_const,lua::bind::return_ref_policy<1>{});
        lua::bind::function(l,"get_self",&test_bind_fields::get_self,lua::bind::return_ref_policy<1>{});
        lua::bind::function(l,"get_count",&test_bind_fields::get_count);
    }
    void test_bind_fields::method1() {
        field3 = field3 + " method1";
    }
}

int luaopen_bind_tests(lua_State* L) {
	lua::state l(L);

	lua::bind::object<tests::test_bind_fields>::register_metatable(l, &tests::test_bind_fields::bind);

    l.createtable();
    lua::bind::object<tests::test_bind_fields>::get_metatable(l);
    l.setfield(-2,"test_bind_fields");

	return 1;
}