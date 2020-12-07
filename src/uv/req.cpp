#include "req.h"
//#include <iostream>


namespace uv {

	req::req() {
		//std::cout << "req::req " << this << std::endl;
		LLAE_DIAG(m_ref_mark=0x4effeed;)
	}

	req::~req() {
		//std::cout << "req::~req " << this << std::endl;
		LLAE_CHECK(m_ref_mark==0x4effeed)
        LLAE_DIAG(m_ref_mark=0x4efbad;)
	}

	void req::attach(uv_req_t* r) {
		uv_req_set_data(r,this);
	}
}