#pragma once

#include <lua/state.h>
#include <string>
#include <cstdint>
#include <optional>
#include "llae/result.h"

namespace autobind_tests {

    /// @luabind
    enum class test_module_enum {
        a,
        b,
        c,
    };

    /// @luabind(prefix=test_module_enum2_)
    enum test_module_enum2 {
        test_module_enum2_a,
        test_module_enum2_b,
        test_module_enum2_c,
    };

    /// @luabind
    struct test_bind_struct {
        /// @luabind
        int x = 0;
        /// @luabind
        int y = 0;

        /// @luabind
        enum class state {
            off,
            on,
        };
    };

    /// @luabind
    struct test_bind_fields {
        static size_t count;
        /// @luabind(raw=true)
        test_bind_fields();
        ~test_bind_fields();
        /// @luabind
        static size_t get_count();
        /// @luabind
        int field1;
        /// @luabind
        float field2;
        /// @luabind(readonly=true)
        int const_field = 5;
        /// @luabind
        std::string field3;
        /// @luabind(policy=field_ref_policy{})
        test_bind_struct field4;
        /// @luabind
        int array1[5];
        /// @luabind(policy=field_ref_policy{})
        test_bind_struct array2[5];
        /// @luabind(policy=string_policy{})
        char string_field[10];
        /// @luabind(policy=string_policy<false>{})
        uint8_t data_field[10];
        /// @luabind
        void method1();
        /// @luabind(policy=return_ref_policy<1>{})
        test_bind_fields* get_self() { return this; }
        /// @luabind(policy=return_ref_policy<1>{})
        test_bind_fields* get_self2(int) { return this; }
        /// @luabind(policy=return_ref_policy<1>{})
        const test_bind_fields* get_const() const { return this; }
        /// @luabind
        void func1(int) const {}
        /// @luabind(policy=return_ref_policy<1>{})
        const test_bind_struct& get_const_field4() const { return field4; }
        /// @luabind(policy=return_ref_policy<1>{})
        test_bind_struct& get_field4() { return field4; }
        /// @luabind(policy=field_ref_policy{})
        std::optional<test_bind_struct> optional_field5;
        /// @luabind(policy=return_ref_policy<1>{})
        const std::optional<test_bind_struct>& get_const_optional_field5() const { return optional_field5; }
        /// @luabind
        llae::result<test_bind_struct> test_result() { 
            if (!optional_field5.has_value()) {
                return llae::string_error::create("failed");
            }
            return llae::result<test_bind_struct>(optional_field5.value());
        }
    };

    /// @luabind
    class zooo {};
    /// @luabind
    class azoo : public zooo {};

    /// @luabind
    static void test_function1(int) {}
}
