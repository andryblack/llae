#pragma once

// This header is not included by autobind_tests.h on purpose: the generated module cpp
// gets it only through @luabind_include, so dev builds prove the feature works.
namespace autobind_tests {

    inline int test_cpp_include_wrapper(int x, int y) {
        return x * 100 + y;
    }

}
