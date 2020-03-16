#include "req.h"
//#include <iostream>


namespace uv {

	req::req() {
		//std::cout << "req::req " << this << std::endl;
	}

	req::~req() {
		//std::cout << "req::~req " << this << std::endl;
	}

	void req::attach(uv_req_t* r) {
		uv_req_set_data(r,this);
	}
}