#include "uv_req_holder.h" 
#include <cassert>

UVReqHolder::UVReqHolder() {

}

UVReqHolder::~UVReqHolder() {
}

void UVReqHolder::attach() {
	get_req()->data = this;
}

void UVReqHolder::on_release() {
	delete this;
}


int UVReqHolder::gc(lua_State* L) {
	Ref<UVReqHolder>* ref = static_cast<Ref<UVReqHolder>*>(lua_touserdata(L,1));
	ref->reset();
	return 0;
}