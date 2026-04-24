#pragma once

#include "extern_autobind_tests.h"

/// @luabind(all=true)
namespace extern_autobind_tests {

    #ifdef LUABIND_PARSE

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

    struct test_bind_struct {
        LUABIND_FIELD(x);
        LUABIND_FIELD(y);

        enum class state {
            off,
            on,
        };
        LUABIND_FIELD(s);
    };

    struct test_bind_fields {
        /// @luabind(raw=true)
        test_bind_fields();
        LUABIND_FUNC(get_count);
        LUABIND_FIELD(field1);
        LUABIND_FIELD(field2);
        /// @luabind(readonly=true)
        LUABIND_FIELD(const_field);
        LUABIND_FIELD(field3);
        /// @luabind(policy=field_ref_policy{})
        LUABIND_FIELD(field4);
        LUABIND_FIELD(array1);
        /// @luabind(policy=field_ref_policy{})
        LUABIND_FIELD(array2);
        /// @luabind(policy=string_policy{})
        LUABIND_FIELD(string_field);
        /// @luabind(policy=string_policy<false>{})
        LUABIND_FIELD(data_field);
        LUABIND_FUNC(method1);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(get_self);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(get_self2);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(get_const);
        LUABIND_FUNC(func1);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(get_const_field4);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(get_field4);
        /// @luabind(policy=field_ref_policy{})
        LUABIND_FIELD(optional_field5);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(get_const_optional_field5);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(set_optional_field5);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(set_optional_field5_val);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(test_result);
    };

    class zooo {};
    class azoo : public zooo {};

    LUABIND_FUNC(test_function1);
    LUABIND_FIELD(test_value);

    #endif
}
