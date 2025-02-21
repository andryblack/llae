#include "object.h"

META_INFO(meta::object, void)

namespace meta {
    const info_t* object::get_class_info() {
        return info<object>::get();
    }
    size_t object::m_count = 0;
}
