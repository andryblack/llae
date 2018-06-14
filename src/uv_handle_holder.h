#ifndef __LLAE_UV_HANDLE_HOLDER_H_INCLUDED__
#define __LLAE_UV_HANDLE_HOLDER_H_INCLUDED__

#include "llae.h"
#include "ref_counter.h"

class UVHandleHolder : public RefCounter {
protected:
	UVHandleHolder();
	virtual ~UVHandleHolder();
	virtual uv_handle_t* get_handle() = 0;
	void attach();
	void push(lua_State* L);
private:
	static void close_and_free_cb(uv_handle_t* handle);
	bool close_int();
	static void close_cb(uv_handle_t* handle);
	virtual void on_release();
	size_t m_lua_refs;
public:
	virtual void on_gc(lua_State* L) {}
	void close();
	static int gc(lua_State* L);
};

#endif /*__LLAE_UV_HANDLE_HOLDER_H_INCLUDED__*/