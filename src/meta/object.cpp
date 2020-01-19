#include "object.h"



namespace meta {
	const info_t object::m_meta_info_storage = { 
        "meta::object", sizeof(meta::object), { nullptr, nullptr } 
    };
}