#pragma once

#include <optional>
#include "intrusive_ptr.h"

namespace common {
    template <typename T>
    struct optional_storage {
        using type = std::optional<T>;
        using result_type = T;
        static result_type& get(type& t) {
            return *t;
        }
        static const result_type& get(const type& t) {
            return *t;
        }
        static bool has_value(const type& t) {
            return t.has_value();
        }
    };

    template <typename T>
    struct optional_storage<common::intrusive_ptr<T>> {
        using type = common::intrusive_ptr<T>;
        using result_type = common::intrusive_ptr<T>;
        static result_type& get(type& t) {
            return t;
        }
        static const result_type& get(const type& t) {
            return t;
        }
        static bool has_value(const type& t) {
            return t;
        }
    };

}