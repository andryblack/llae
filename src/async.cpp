#include "async.h"


AsyncBase::AsyncBase(uv_loop_t* loop) {
	uv_async_init(loop,&m_async,&async_cb);
	attach();
}

uv_handle_t* AsyncBase::get_handle() {
	return reinterpret_cast<uv_handle_t*>(&m_async);
}

void AsyncBase::async_cb(uv_async_t* handle) {
	static_cast<AsyncBase*>(handle->data)->on_async();
}

void AsyncBase::async_send() {
	uv_async_send(&m_async);
}