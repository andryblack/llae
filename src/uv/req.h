#ifndef __LLAE_UV_REQ_H_INCLUDED__
#define __LLAE_UV_REQ_H_INCLUDED__

#include "common/ref_counter.h"
#include "decl.h"

namespace uv {

	class req : public common::ref_counter_base {
	protected:
		void attach(uv_req_t* r);
		req();
		~req();
	};

}

#endif /*__LLAE_UV_REQ_H_INCLUDED__*/