#pragma once

#include "extern_autobind_tests.h"

namespace extern_autobind_tests {

    #ifdef LUABIND_PARSE

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
        LUABIND_FIELD(x);
        /// @luabind
        LUABIND_FIELD(y);

        /// @luabind
        enum class state {
            off,
            on,
        };
        /// @luabind
        LUABIND_FIELD(s);
    };

    /// @luabind
    struct test_bind_fields {
        /// @luabind(raw=true)
        test_bind_fields();
        /// @luabind
        LUABIND_FUNC(get_count);
        /// @luabind
        LUABIND_FIELD(field1);
        /// @luabind
        LUABIND_FIELD(field2);
        /// @luabind(readonly=true)
        LUABIND_FIELD(const_field);
        /// @luabind
        LUABIND_FIELD(field3);
        /// @luabind(policy=field_ref_policy{})
        LUABIND_FIELD(field4);
        /// @luabind
        LUABIND_FIELD(array1);
        /// @luabind(policy=field_ref_policy{})
        LUABIND_FIELD(array2);
        /// @luabind(policy=string_policy{})
        LUABIND_FIELD(string_field);
        /// @luabind(policy=string_policy<false>{})
        LUABIND_FIELD(data_field);
        /// @luabind
        LUABIND_FUNC(method1);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(get_self);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(get_self2);
        /// @luabind(policy=return_ref_policy<1>{})
        LUABIND_FUNC(get_const);
        /// @luabind
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

    /// @luabind
    class zooo {};
    /// @luabind
    class azoo : public zooo {};

    /// @luabind
    LUABIND_FUNC(test_function1);
    /// @luabind
    LUABIND_FIELD(test_value);

    #endif
}
