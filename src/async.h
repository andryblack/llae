#ifndef __LLAE_ASYNC_H_INCLUDED__
#define __LLAE_ASYNC_H_INCLUDED__

#include "uv_handle_holder.h"

class AsyncBase : public UVHandleHolder {
protected:
	uv_async_t	m_async;
	static void async_cb(uv_async_t* handle);
	virtual void on_async() = 0;
public:
	explicit AsyncBase(uv_loop_t* loop);
	uv_handle_t* get_handle();
	void async_send();
};

#endif /*__LLAE_ASYNC_H_INCLUDED__*/