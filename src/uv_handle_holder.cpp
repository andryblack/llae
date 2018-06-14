#include "uv_handle_holder.h" 
#include <cassert>

UVHandleHolder::UVHandleHolder() : m_lua_refs(0)  {

}

UVHandleHolder::~UVHandleHolder() {
	assert(m_lua_refs == 0);
}

void UVHandleHolder::attach() {
	get_handle()->data = this;
}

void UVHandleHolder::on_release() {
	if (close_int()) {
		delete this;
	}
}

void UVHandleHolder::close_and_free_cb(uv_handle_t* handle) {
	assert(handle->data);
	UVHandleHolder* holder = static_cast<UVHandleHolder*>(handle->data);
	handle->data = 0;
	delete holder;
}

void UVHandleHolder::close_cb(uv_handle_t* handle) {
	assert(handle->data);
	UVHandleHolder* holder = static_cast<UVHandleHolder*>(handle->data);
	handle->data = 0;
	holder->remove_ref();
}

bool UVHandleHolder::close_int() {
	uv_handle_t* handle = get_handle();
	if (handle->data) {
		uv_close(handle,&close_and_free_cb);
		return false;
	}
	return true;
}

void UVHandleHolder::close() {
	uv_handle_t* handle = get_handle();
	if (handle->data && !uv_is_closing(handle)) {
		add_ref();
		uv_close(handle,&close_cb);
	}
}

void UVHandleHolder::push(lua_State*) {
	++m_lua_refs;
}
int UVHandleHolder::gc(lua_State* L) {
	Ref<UVHandleHolder>* ref = static_cast<Ref<UVHandleHolder>*>(lua_touserdata(L,1));
	if (ref) {
		UVHandleHolder* holder = ref->get();
		if (holder) {
			assert(holder->m_lua_refs);
			--holder->m_lua_refs;
			if (!holder->m_lua_refs) {
				holder->on_gc(L);
			}
		}
		ref->reset();
	}
	return 0;
}