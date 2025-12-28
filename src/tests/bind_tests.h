#pragma once

#include <lua/state.h>
#include <string>
#include <cstdint>

namespace tests {

    struct test_bind_struct {
        int x = 0;
        int y = 0;
        static void lbind(lua::state& l);
    };

    struct test_bind_fields {
        static size_t count;
        test_bind_fields();
        ~test_bind_fields();
        static size_t get_count();
        int field1;
        float field2;
        int const_field = 5;
        std::string field3;
        test_bind_struct field4;
        int array1[5];
        test_bind_struct array2[5];
        char string_field[10];
        uint8_t data_field[10];
        void method1();
        test_bind_fields* get_self() { return this; }
        test_bind_fields* get_self2(int) { return this; }
        const test_bind_fields* get_const() const { return this; }
        void func1(int) const {}
        static void bind(lua::state& l);
    };

}