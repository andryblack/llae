#ifndef __LLAE_UV_REQ_HOLDER_H_INCLUDED__
#define __LLAE_UV_REQ_HOLDER_H_INCLUDED__

#include "llae.h"
#include "ref_counter.h"

class UVReqHolder : public RefCounter {
protected:
	UVReqHolder();
	virtual ~UVReqHolder();
	virtual uv_req_t* get_req() = 0;
	void attach();
private:
	virtual void on_release();
	static int gc(lua_State* L);
};

#endif /*__LLAE_UV_REQ_HOLDER_H_INCLUDED__*/