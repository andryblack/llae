#include "req.h"


META_OBJECT_INFO(uv::req,meta::object)


namespace uv {

	void req::attach(uv_req_t* r) {
		uv_req_set_data(r,this);
	}
}