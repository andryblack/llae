#ifndef __LLAE_FILE_H_INCLUDED__
#define __LLAE_FILE_H_INCLUDED__

#include <uv.h>
#include "uv_req_holder.h"
#include "lua_holder.h"
#include "luabind.h"
#include "stream.h"

class FSReq : public UVReqHolder {
protected:
	uv_fs_t m_req;
	virtual void on_cb();
	static void fs_req_cb(uv_fs_t* req);
	FSReq();
	virtual uv_req_t* get_req();	
public:
	uv_fs_t* get_fs_req() { return &m_req; }
};

class FSLuaReq : public FSReq {
protected:
	LuaFunction m_cb;
	virtual void on_cb();
	virtual void on_result(lua_State* L,const char* err);
};

class FSLuaThreadReq : public FSReq {
protected:
	LuaThread m_cb;
	virtual void on_cb();
	virtual void on_result(lua_State* L,const char* err);
};

class File;
typedef Ref<File> FileRef;

class FileStatReq : public FSLuaReq {
protected:
	virtual void on_result(lua_State* L,const char* err);
public:
	int stat(lua_State* L,const char* path,const luabind::function& f); 
};
typedef Ref<FileStatReq> FileStatReqRef;

class FileStatThreadReq : public FSLuaThreadReq {
protected:
	virtual void on_result(lua_State* L,const char* err);
public:
	int stat(lua_State* L,const char* path,const luabind::thread& f); 
};
typedef Ref<FileStatThreadReq> FileStatThreadReqRef;


class FileMkdirReq : public FSLuaReq {
public:
	int mkdir(lua_State* L,const char* path,const luabind::function& f); 
};
typedef Ref<FileMkdirReq> FileMkdirReqRef;

class FileMkdirThreadReq : public FSLuaThreadReq {
public:
	int mkdir(lua_State* L,const char* path,const luabind::thread& f); 
};
typedef Ref<FileMkdirThreadReq> FileMkdirThreadReqRef;

class FileScandirReq : public FSLuaReq {
public:
	int scandir(lua_State* L,const char* path,const luabind::function& f); 
	virtual void on_result(lua_State* L,const char* err);
};
typedef Ref<FileScandirReq> FileScandirReqRef;

class FileScandirThreadReq : public FSLuaThreadReq {
public:
	int scandir(lua_State* L,const char* path,const luabind::thread& f); 
	virtual void on_result(lua_State* L,const char* err);
};
typedef Ref<FileScandirThreadReq> FileScandirThreadReqRef;

class FileCloseReq : public FSReq {
private:
	FileRef m_file;
public:
	explicit FileCloseReq( const FileRef& file );
	~FileCloseReq();
	int close(uv_loop_t* loop,uv_file f);
};


class FSReadPipeReq;

class FSReadPipe : public RefCounter {
private:
	FileRef m_file;
	int64_t m_pos;
	int64_t m_size;
protected:
	LuaFunction m_cb;
protected:
	static const size_t BUFFER_SIZE = 1024*128; 
	virtual uv_buf_t* alloc_buffer() = 0;
public:
	explicit FSReadPipe(const FileRef& ref,int64_t offset,int64_t size);
	//uv_buf_t* get_buf() { return &m_uv_buf; }
	const FileRef& get_file() const { return m_file; }
	void on_read(FSReadPipeReq& req);
	int start_read(uv_loop_t* loop);
	virtual int on_data(uv_loop_t* loop,int64_t size);
	virtual void on_read_end(uv_loop_t* loop);
};
typedef Ref<FSReadPipe> FSReadPipeRef;


class FSReadPipeReq : public FSReq {
private:
	FSReadPipeRef m_pipe;
public:
	explicit FSReadPipeReq(const FSReadPipeRef& pipe);
	~FSReadPipeReq();
	int read(uv_loop_t* loop,int64_t offset,uv_buf_t* buf);
	virtual void on_cb();
};
typedef Ref<FSReadPipeReq> FSReadPipeReqRef;

class FileOpenReq : public FSLuaReq {
public:
	int open(lua_State* L,const char* path,int flags,int mode,const luabind::function& f);
	virtual void on_result(lua_State* L,const char* err);
};
typedef Ref<FileOpenReq> FileOpenReqRef;

class Stream;
typedef Ref<Stream> StreamRef;

class FileSendPipe : public FSReadPipe {
private:
	StreamRef m_stream;
	MemWriteReqRef m_write;
protected:
	virtual uv_buf_t* alloc_buffer();
public:
	explicit FileSendPipe(const FileRef& ref,const StreamRef& stream);
	~FileSendPipe();
	int send(lua_State* L,const luabind::function& f);
	virtual int on_data(uv_loop_t* loop,int64_t size);
};
typedef Ref<FileSendPipe> FileSendPipeRef;

class File : public RefCounter {
	uv_file m_file;
	uv_loop_t* m_loop;
protected:
	void on_release();
public:
	explicit File(uv_file f,uv_loop_t* loop);
	uv_file get_file() { return m_file; }

	void push(lua_State* L);

	void send(lua_State* L,const StreamRef& stream, const luabind::function& f);

	static void lbind(lua_State* L);

	static int open(lua_State* L);
	static int stat(lua_State* L);
	static int mkdir(lua_State* L);
	static int scandir(lua_State* L);
};

#endif /*__LLAE_FILE_H_INCLUDED__*/