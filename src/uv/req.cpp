#include "req.h"



namespace uv {

	void req::attach(uv_req_t* r) {
		uv_req_set_data(r,this);
	}
}