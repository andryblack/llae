#include "handle.h"
//#include <iostream>

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
		//std::cout << "handle closed for destroy << " << h << std::endl;
		handle* self = static_cast<handle*>(uv_handle_get_data(h));
		uv_handle_set_data(h,nullptr);
		delete self;
	}

	void handle::close_cb(uv_handle_t* h) {
		//std::cout << "handle closed << " << h << std::endl;
		handle* self = static_cast<handle*>(uv_handle_get_data(h));
        uv_handle_set_data(h,nullptr);
		if (self) {
			self->on_closed();
			self->remove_ref();
		}
		
	}

	void handle::destroy() {
		handle* self = static_cast<handle*>(uv_handle_get_data(get_handle()));
		if (!self) {
			return;
		}
		if (!uv_is_closing(get_handle())) {
            on_closed();
			uv_close(get_handle(),&handle::close_destroy_cb);
		} else {
			meta::object::destroy(); 
		}
	}

	void handle::close() {
		handle* self = static_cast<handle*>(uv_handle_get_data(get_handle()));
		if (!self) {
			return;
		}
		if (!uv_is_closing(get_handle())) {
			add_ref();
			uv_close(get_handle(),&handle::close_cb);
		}
	}

	void handle::unref() {
		uv_unref(get_handle());
	}
}
