#include "extern_autobind_tests.h"
#include <lua/bind.h>

META_INFO(extern_autobind_tests::test_bind_struct,void)
META_INFO(extern_autobind_tests::test_bind_fields,void)

META_INFO(extern_autobind_tests::zooo,void)
META_INFO(extern_autobind_tests::azoo,extern_autobind_tests::zooo)

namespace extern_autobind_tests {

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

    void test_bind_fields::method1() {
        field3 = field3 + " method1";
    }
}
