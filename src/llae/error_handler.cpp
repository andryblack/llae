#include "error_handler.h"
#include "app.h"

META_OBJECT_INFO(llae::error_handler,meta::object)

namespace llae {

	void error_handler::handle_error(app& a,lua::status e) {
		a.stop(1);
	}

}