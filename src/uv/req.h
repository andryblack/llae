#ifndef __LLAE_UV_REQ_H_INCLUDED__
#define __LLAE_UV_REQ_H_INCLUDED__

#include "meta/object.h"
#include "decl.h"

namespace uv {

	class req : public meta::object {
		META_OBJECT
	protected:
		void attach(uv_req_t* r);
	};

}

#endif /*__LLAE_UV_REQ_H_INCLUDED__*/