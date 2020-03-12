#include "handle.h"

META_OBJECT_INFO(uv::handle,meta::object)

namespace uv {

	//static int handle_destoyed_mark = 0xdeadbeef;

	handle::handle() {
	}

	handle::~handle() {
	}

	void handle::attach() {
		uv_handle_set_data(get_handle(),this);
	}

	void handle::close_destroy_cb(uv_handle_t* h) {
		handle* self = static_cast<handle*>(uv_handle_get_data(h));
		uv_handle_set_data(h,nullptr);
		delete self;
	}

	void handle::close_cb(uv_handle_t* h) {
		handle* self = static_cast<handle*>(uv_handle_get_data(h));
		if (self) {
			self->on_closed();
			self->remove_ref();
		}
		uv_handle_set_data(h,nullptr);
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
			add_ref();
			uv_close(get_handle(),&handle::close_cb);
		}
	}
}
