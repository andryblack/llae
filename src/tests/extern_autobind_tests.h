#pragma once

#include <lua/state.h>
#include <string>
#include <cstdint>
#include <optional>
#include "llae/result.h"

namespace extern_autobind_tests {

    enum class test_module_enum {
        a,
        b,
        c,
    };

    enum test_module_enum2 {
        test_module_enum2_a,
        test_module_enum2_b,
        test_module_enum2_c,
    };

    struct test_bind_struct {
        int x = 0;
        int y = 0;

        enum class state {
            off,
            on,
        };
        state s;
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
        const test_bind_struct& get_const_field4() const { return field4; }
        test_bind_struct& get_field4() { return field4; }
        std::optional<test_bind_struct> optional_field5;
        const std::optional<test_bind_struct>& get_const_optional_field5() const { return optional_field5; }
        void set_optional_field5(const std::optional<test_bind_struct>& v) { optional_field5 = v; }
        void set_optional_field5_val(std::optional<test_bind_struct> v) { optional_field5 = v; }
        llae::result<test_bind_struct> test_result() {
            if (!optional_field5.has_value()) {
                return llae::string_error::create("failed");
            }
            return llae::result<test_bind_struct>(optional_field5.value());
        }
    };

    class zooo {};
    class azoo : public zooo {};

    static void test_function1(int) {}

    constexpr int test_value = 123;
}
