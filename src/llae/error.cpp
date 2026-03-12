#include "error.h"
#include "lua/state.h"
#include "lua/bind.h"
#include <cstdio>

META_OBJECT_INFO(llae::error,meta::object)
META_OBJECT_INFO(llae::string_error,llae::error)
META_OBJECT_INFO(llae::code_error,llae::error)

namespace llae {

	const std::string error::default_category = "llae";

	std::string error::to_string() const {
		return std::string("[") + get_category() + "]";
	}

	std::string string_error::to_string() const {
		return error::to_string() + ":" + m_message;
	}

	std::string code_error::to_string() const {
		char buf[128];
		::snprintf(buf,sizeof(buf),"[%s]:%d",get_category().c_str(),get_code());
		return std::string(buf);
	}


}