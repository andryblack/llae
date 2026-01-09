#pragma once

#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "lua/types.h"

namespace llae {

	class app;
	class error_handler : public meta::object {
		META_OBJECT
	public:
		virtual void handle_error(app& a,lua::status e);
	};
	using error_handler_ptr = common::intrusive_ptr<error_handler>;

}