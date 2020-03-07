#include "handle.h"

META_OBJECT_INFO(uv::handle,meta::object)

namespace uv {

	handle::handle() {

	}

	handle::~handle() {

	}

	void handle::attach() {
		uv_handle_set_data(get_handle(),this);
	}

	void handle::close_destroy_cb(uv_handle_t* h) {
		handle* self = static_cast<handle*>(uv_handle_get_data(h));
		delete self;
	}

	void handle::close_cb(uv_handle_t* h) {
		// closed
	}

	void handle::destroy() {
		if (!uv_is_closing(get_handle())) {
			uv_close(get_handle(),&handle::close_destroy_cb);
		} else {
			meta::object::destroy(); 
		}
	}

	void handle::close() {
		if (!uv_is_closing(get_handle())) {
			uv_close(get_handle(),&handle::close_cb);
		}
	}
}