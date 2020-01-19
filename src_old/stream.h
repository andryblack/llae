#ifndef __LLAE_STREAM_H_INCLUDED__
#define __LLAE_STREAM_H_INCLUDED__

#include "llae.h"
#include "uv_handle_holder.h"
#include "uv_req_holder.h"
#include "lua_holder.h"
#include <vector>

#include "luabind.h"
namespace luabind {
	template <>
	struct S<const uv_buf_t*> {
		static void push(lua_State* L,const uv_buf_t* buf) {
			lua_pushlstring(L,buf->base,buf->len);
		}
	};
	template <>
	struct S<uv_buf_t> {
		static void get(lua_State* L,int idx,uv_buf_t* buf) {
			size_t len = 0;
			buf->base = const_cast<char*>(luaL_checklstring(L,idx,&len));
			buf->len = len;
		}
	};
}

class WriteReqBase : public UVReqHolder {
protected:
	uv_write_t m_write;
	static void write_cb(uv_write_t* req, int status);
	virtual void on_end(int status);
	virtual uv_buf_t* get_buffers() = 0;
	virtual size_t get_buffers_count() = 0;
public:
	WriteReqBase();
	virtual uv_req_t* get_req();
	int write(uv_stream_t* stream);
};
class WriteReq : public WriteReqBase {
private:
	std::vector<uv_buf_t> m_buffers;
	std::vector<LuaHolder> m_data;
	virtual void on_end(int status);
	virtual uv_buf_t* get_buffers() { return m_buffers.data();}
	virtual size_t get_buffers_count() { return m_buffers.size(); }
public:
	explicit WriteReq();
	void put(lua_State* L,int idx);
};
typedef Ref<WriteReq> WriteReqRef;

class MemWriteReq : public WriteReqBase {
private:
	std::vector<char> m_data;
	uv_buf_t m_buf;
	virtual uv_buf_t* get_buffers() { return &m_buf;}
	virtual size_t get_buffers_count() { return 1; }
public:
	explicit MemWriteReq();
	uv_buf_t* alloc(size_t size);
	void resize(size_t size);
};
typedef Ref<MemWriteReq> MemWriteReqRef;

class Stream;
typedef Ref<Stream> StreamRef;
class ShootdownReq : public UVReqHolder {
private:
	uv_shutdown_t m_req;
	StreamRef m_stream;
	static void shutdown_cb(uv_shutdown_t* req, int status);
	void on_end(int status);
public:
	explicit ShootdownReq();
	virtual uv_req_t* get_req();
	int start(const StreamRef& stream);
};
typedef Ref<ShootdownReq> ShootdownReqPtr;

class ReadRequest : public RefCounter {
public:
	virtual void on_alloc(Stream* s,size_t suggested_size, uv_buf_t* buf);
	virtual void on_read(Stream* s,ssize_t nread, const uv_buf_t* buf);
};
typedef Ref<ReadRequest> ReadRequestRef;

class ThreadReadRequest : public ReadRequest {
protected:
	LuaThread m_read_th;
public:
	virtual void on_read(Stream* s,ssize_t nread, const uv_buf_t* buf);
	void start(lua_State* L);
};
typedef Ref<ThreadReadRequest> ThreadReadRequestRef;

class Stream : public UVHandleHolder {
protected:
	static const char* get_mt_name();
	virtual uv_handle_t* get_handle();
	
	ReadRequestRef m_read_req;

	void on_alloc(size_t suggested_size, uv_buf_t* buf);
	void on_read(ssize_t nread, const uv_buf_t* buf);
	void on_gc(lua_State *L);
	
private:	
	static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
public:
	explicit Stream();
	virtual uv_stream_t* get_stream() = 0;
	virtual void push(lua_State* L) = 0;
	static void lbind(lua_State* L);
	int start_stream_read(const ReadRequestRef& req);
	static int read(lua_State* L);
	void write(lua_State* L);
	void shutdown(lua_State* L);
	void close();
};
typedef Ref<Stream> StreamRef;



#endif /*__LLAE_STREAM_H_INCLUDED__*/