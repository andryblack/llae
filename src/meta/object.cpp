#include "object.h"



namespace meta {
	static const info_t object_info = { 
        "meta::object", nullptr
    };
    const info_t* object::get_class_info() { return &object_info; }
}