#pragma once

#include "extern_autobind_tests.h"

namespace extern_autobind_tests {

    /** @extern
    enum class test_module_enum {
        a,
        b,
        c,
    };
    */

    /** @extern
    /// @luabind(prefix=test_module_enum2_)
    enum test_module_enum2 {
        test_module_enum2_a,
        test_module_enum2_b,
        test_module_enum2_c,
    };
    */

    /** @extern
    struct test_bind_struct {
        @field(x);
        @field(y);

        enum class state {
            off,
            on,
        };
        state s;
    };
    */

    /** @extern
    struct test_bind_fields {
        /// @luabind(raw=true)
        test_bind_fields();
        @func(get_count)
        @field(field1);
        @field(field2);
        @field(const_field,readonly=true);
        @field(field3);
        @field(field4,policy=field_ref_policy{});
        @field(array1);
        @field(array2,policy=field_ref_policy{});
        @field(string_field,policy=string_policy{});
        @field(data_field,policy=string_policy<false>{});
        @func(method1);
        @func(get_self,policy=return_ref_policy<1>{});
        @func(get_self2,policy=return_ref_policy<1>{});
        @func(get_const,policy=return_ref_policy<1>{});
        @func(func1);
        @func(get_const_field4,policy=return_ref_policy<1>{});
        @func(get_field4,policy=return_ref_policy<1>{});
        @field(optional_field5,policy=field_ref_policy{});
        @func(get_const_optional_field5,policy=return_ref_policy<1>{});
        @func(set_optional_field5,policy=return_ref_policy<1>{});
        @func(set_optional_field5_val,policy=return_ref_policy<1>{});
        @func(test_result,policy=return_ref_policy<1>{});
    };
    */

    /** @extern
    class zooo {};
    class azoo : public zooo {};
    @func(test_function1)
    @field(test_value);
    */
}
