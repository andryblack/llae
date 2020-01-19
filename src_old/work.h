#ifndef _LLAE_LLAE_WORK_H_INCLUDED_
#define _LLAE_LLAE_WORK_H_INCLUDED_

#include "uv_req_holder.h"
#include "lua_holder.h"

class WorkReq : public UVReqHolder {
protected:
	uv_work_t m_work;
	virtual void on_work() = 0;
	virtual void on_after_work(int status) = 0;
	static void work_cb(uv_work_t* req);
	static void after_work_cb(uv_work_t* req,int status);
	WorkReq();
	virtual uv_req_t* get_req();	
public:
	int start(uv_loop_t* loop);
};
typedef Ref<WorkReq> WorkReqRef;

class ThreadWorkReq : public WorkReq {
protected:
	LuaThread m_th;
public:
	virtual void on_after_work(int status) ;
public:
	int start(lua_State* L);
};
typedef Ref<ThreadWorkReq> ThreadWorkReqRef;

#endif /*_LLAE_LLAE_WORK_H_INCLUDED_*/