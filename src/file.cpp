#include "file.h"
#include "stream.h"
#include <iostream>
#include <cassert>

#ifndef UV_FS_O_RDONLY
#define UV_FS_O_RDONLY O_RDONLY 
#endif

static void push_timespec(lua_State* L,const uv_timespec_t& ts) {
	lua_newtable(L);
	lua_pushinteger(L,ts.tv_sec);
	lua_setfield(L,-2,"sec");
	lua_pushinteger(L,ts.tv_nsec);
	lua_setfield(L,-2,"nsec");
}

FSReq::FSReq() {
	attach();
}

uv_req_t* FSReq::get_req() {
	return reinterpret_cast<uv_req_t*>(&m_req);
}

void FSReq::on_cb() {
	
}

void FSReq::fs_req_cb(uv_fs_t* req) {
	FSReq* r = static_cast<FSReq*>(req->data);
	r->on_cb();
	uv_fs_req_cleanup(req);
	r->remove_ref();
}

void FSLuaReq::on_cb() {
	uv_loop_t* loop = m_req.loop;
	lua_State* L = llae_get_vm(loop);
	if (L && m_cb) {
		const char* err = 0;
		if (m_req.result < 0) {
			err = uv_strerror(m_req.result);
		}
		on_result(L,err);
		m_cb.reset(L);
	}
}

void FSLuaReq::on_result(lua_State* L,const char* err) {
	if (m_cb) {
		m_cb.callv(L,"FSReq::on_result",err==0,err);
	}
}


void FSLuaThreadReq::on_cb() {
	uv_loop_t* loop = m_req.loop;
	lua_State* L = llae_get_vm(loop);
	if (L && m_cb) {
		const char* err = 0;
		if (m_req.result < 0) {
			err = uv_strerror(m_req.result);
		}
		on_result(L,err);
		m_cb.reset(L);
	}
}

void FSLuaThreadReq::on_result(lua_State* L,const char* err) {
	if (m_cb) {
		m_cb.resumev(L,"FSLuaThreadReq::on_result",err==0,err);
	}
}




int FileStatReq::stat(lua_State* L,const char* path,const luabind::function& f) {
	m_cb.assign(L,f);
	int r = uv_fs_stat(llae_get_loop(L),&m_req,path,&FSReq::fs_req_cb);
	if (r < 0) {
		m_cb.reset(L);
	} else {
		add_ref();
	}
	return r;
}

static void push_stat(lua_State* L,const uv_stat_t& statbuf) {
	lua_newtable(L);
	push_timespec(L,statbuf.st_mtim);
	lua_setfield(L,-2,"mtim");
	lua_pushinteger(L,statbuf.st_size);
	lua_setfield(L,-2,"size");
	int fmt = statbuf.st_mode & S_IFMT;
	if (fmt == S_IFREG) {
		lua_pushboolean(L,1);
		lua_setfield(L,-2,"isfile");
	} else if (fmt == S_IFDIR) {
		lua_pushboolean(L,1);
		lua_setfield(L,-2,"isdir");
	}
}

void FileStatReq::on_result(lua_State* L,const char* err) {
	if (err) {
		FSLuaReq::on_result(L,err);
		return;
	}
	push_stat(L,m_req.statbuf);
	m_cb.callv(L,"FSReq::on_result",luabind::table(lua_absindex(L,-1)),err);
}


int FileStatThreadReq::stat(lua_State* L,const char* path,const luabind::thread& f) {
	m_cb.assign(L,f);
	int r = uv_fs_stat(llae_get_loop(L),&m_req,path,&FSReq::fs_req_cb);
	if (r < 0) {
		m_cb.reset(L);
	} else {
		add_ref();
	}
	return r;
}

void FileStatThreadReq::on_result(lua_State* L,const char* err) {
	if (err) {
		FSLuaThreadReq::on_result(L,err);
		return;
	}
	push_stat(L,m_req.statbuf);
	luabind::check_type(L,-1,LUA_TTABLE);
	m_cb.resumev(L,"FSLuaThreadReq::on_result",luabind::table(lua_absindex(L,-1)));
}


int FileMkdirReq::mkdir(lua_State* L,const char* path,const luabind::function& f) {
	m_cb.assign(L,f);
	int r = uv_fs_mkdir(llae_get_loop(L),&m_req,path,0700,&FSReq::fs_req_cb);
	if (r < 0) {
		m_cb.reset(L);
	} else {
		add_ref();
	}
	return r;
}

int FileMkdirThreadReq::mkdir(lua_State* L,const char* path,const luabind::thread& f) {
	m_cb.assign(L,f);
	int r = uv_fs_mkdir(llae_get_loop(L),&m_req,path,0700,&FSReq::fs_req_cb);
	if (r < 0) {
		m_cb.reset(L);
	} else {
		add_ref();
	}
	return r;
}


static void push_scandir(lua_State* L,uv_fs_t* req) {
	lua_newtable(L);
	lua_Integer idx = 1;
	while(true) {
		uv_dirent_t ent;
		int r = uv_fs_scandir_next(req,&ent);
		if (r == UV_EOF) {
			return;
		}
		if (ent.type == UV_DIRENT_FILE || ent.type == UV_DIRENT_DIR) {
			lua_newtable(L);
			lua_pushstring(L,ent.name);
			lua_setfield(L,-2,"name");
			if (ent.type == UV_DIRENT_FILE) {
				lua_pushboolean(L,1);
				lua_setfield(L,-2,"isfile");
			} else if (ent.type == UV_DIRENT_DIR) {
				lua_pushboolean(L,1);
				lua_setfield(L,-2,"isdir");
			}
			lua_seti(L,-2,idx);
			++idx;
		}
	}
}

int FileScandirReq::scandir(lua_State* L,const char* path,const luabind::function& f) {
	m_cb.assign(L,f);
	int r = uv_fs_scandir(llae_get_loop(L),&m_req,path,0,&FSReq::fs_req_cb);
	if (r < 0) {
		m_cb.reset(L);
	} else {
		add_ref();
	}
	return r;
}

void FileScandirReq::on_result(lua_State* L,const char* err) {
	if (err) {
		FSLuaReq::on_result(L,err);
		return;
	}
	push_scandir(L,&m_req);
	assert(lua_istable(L,-1));
	m_cb.callv(L,"FileScandirReq::on_result",luabind::table(lua_absindex(L,-1)),err);
}

int FileScandirThreadReq::scandir(lua_State* L,const char* path,const luabind::thread& f) {
	m_cb.assign(L,f);
	int r = uv_fs_scandir(llae_get_loop(L),&m_req,path,0,&FSReq::fs_req_cb);
	//printf("FileScandirThreadReq::scandir %d\n", r);
	if (r < 0) {
		m_cb.reset(L);
	} else {
		add_ref();
	}
	return r;
}

void FileScandirThreadReq::on_result(lua_State* L,const char* err) {
	if (err) {
		FSLuaThreadReq::on_result(L,err);
		return;
	}
	//printf("FileScandirThreadReq::on_result\n");
	push_scandir(L,&m_req);
	m_cb.resumev(L,"FileScandirThreadReq::on_result",luabind::table(lua_absindex(L,-1)));
}

int File::stat(lua_State* L) {
	const char* path = luaL_checkstring(L,1);
	if (lua_isfunction(L,2)) {
		// async
		luabind::function f = luabind::S<luabind::function>::get(L,2);
		FileStatReqRef req(new FileStatReq());
		int res = req->stat(L,path,f);
		lua_llae_handle_error(L,"File::stat",res);
	} else if (lua_isyieldable(L)) {
		// async thread
		lua_pushthread(L);
		luabind::thread th = luabind::S<luabind::thread>::get(L,-1);
		FileStatThreadReqRef req(new FileStatThreadReq());
		int res = req->stat(L,path,th);
		lua_llae_handle_error(L,"File::stat",res);
		lua_yield(L,0);
	} else {
		luaL_error(L,"sync File::stat not supported");
	}
	return 0;
}


int File::mkdir(lua_State* L) {
	const char* path = luaL_checkstring(L,1);
	if (lua_isfunction(L,2)) {
		// async
		luabind::function f = luabind::S<luabind::function>::get(L,2);
		FileMkdirReqRef req(new FileMkdirReq());
		int res = req->mkdir(L,path,f);
		lua_llae_handle_error(L,"File::mkdir",res);
	} else if (lua_isyieldable(L)) {
		// async thread
		lua_pushthread(L);
		luabind::thread th = luabind::S<luabind::thread>::get(L,-1);
		FileMkdirThreadReqRef req(new FileMkdirThreadReq());
		int res = req->mkdir(L,path,th);
		if (res < 0) {
			lua_pushnil(L);
			lua_pushstring(L,uv_strerror(res));
			return 2;
		}
		lua_yield(L,0);
	} else {
		luaL_error(L,"sync File::mkdir not supported");
	}
	return 0;
}

int File::scandir(lua_State* L) {
	const char* path = luaL_checkstring(L,1);
	if (lua_isfunction(L,2)) {
		// async
		luabind::function f = luabind::S<luabind::function>::get(L,2);
		FileScandirReqRef req(new FileScandirReq());
		int res = req->scandir(L,path,f);
		lua_llae_handle_error(L,"File::mkdir",res);
	} else if (lua_isyieldable(L)) {
		// async thread
		lua_pushthread(L);
		luabind::thread th = luabind::S<luabind::thread>::get(L,-1);
		FileScandirThreadReqRef req(new FileScandirThreadReq());
		int res = req->scandir(L,path,th);
		if (res < 0) {
			lua_pushnil(L);
			lua_pushstring(L,uv_strerror(res));
			return 2;
		}
		lua_yield(L,0);
	} else {
		luaL_error(L,"sync File::scandir not supported");
	}
	return 0;
}

FileCloseReq::FileCloseReq(const FileRef& file) : m_file(file) {

}

FileCloseReq::~FileCloseReq() {

}

int FileCloseReq::close(uv_loop_t* loop,uv_file f) {
	int r = uv_fs_close(loop,&m_req,f,&FSReq::fs_req_cb);
	if (r>=0) {
		add_ref();
	}
	return r;
}

FSReadPipe::FSReadPipe(const FileRef& file,int64_t offset,int64_t size) 
	: m_file(file),m_pos(offset),m_size(size) {
}

int FSReadPipe::on_data(uv_loop_t* loop,int64_t size) {
	// lua_State* L = llae_get_vm(loop);
	// if (L && m_cb) {
	// 	m_cb.callv(L,"FSReadPipe::on_data",luabind::lnil(),get_buf());	
	// }
	return 0;
}

void FSReadPipe::on_read_end(uv_loop_t* loop) {
	lua_State* L = llae_get_vm(loop);
	if (L && m_cb) {
		m_cb.callv(L,"FSReadPipe::on_read_end",luabind::lnil());	
		m_cb.reset(L);
	}
}

void FSReadPipe::on_read(FSReadPipeReq& req) {
	uv_fs_t* r = req.get_fs_req();
	//std::cout << "FSReadPipe::on_read " << r->result << std::endl;
	if (r->result < 0) {
		lua_State* L = llae_get_vm(r->loop);
		if (L && m_cb) {
			m_cb.callv(L,"FSReadPipe::on_read",uv_strerror(r->result));	
			m_cb.reset(L);
		}
		remove_ref();
		return;
	}
	int64_t size = r->result;
	if (size == 0) {
		on_read_end(r->loop);
		remove_ref();
		return;
	} 
	if (m_pos != -1) {
		m_pos += size;
	}
	
	int dr = on_data(r->loop,size);
	if ( dr < 0) {
		lua_State* L = llae_get_vm(r->loop);
		if (L && m_cb) {
			m_cb.callv(L,"FSReadPipe::on_read",uv_strerror(dr));	
			m_cb.reset(L);
		}
		remove_ref();
		return;
	}
	if (m_pos != -1 && m_pos >= m_size) {
		on_read_end(r->loop);
		remove_ref();
		return;
	}

	uv_buf_t* buf = alloc_buffer();

	if (m_pos != -1) {
		int64_t s = size - m_pos;
		if (buf->len > s) {
			buf->len = s;
		}
	} 
	FSReadPipeReqRef rreq(new FSReadPipeReq(FSReadPipeRef(this)));
	int rr = rreq->read(r->loop,m_pos,buf);
	if (rr < 0) {
		lua_State* L = llae_get_vm(r->loop);
		if (L && m_cb) {
			m_cb.callv(L,"FSReadPipe::on_read",uv_strerror(rr));
			m_cb.reset(L);
		}
		remove_ref();
	}
	
}

FSReadPipeReq::FSReadPipeReq(const FSReadPipeRef& pipe) : m_pipe(pipe)  {
	//std::cout << "FSReadPipeReq::FSReadPipeReq" << std::endl;
}

FSReadPipeReq::~FSReadPipeReq() {
	//std::cout << "FSReadPipeReq::~FSReadPipeReq" << std::endl;
}

int FSReadPipeReq::read(uv_loop_t* loop,int64_t offset,uv_buf_t* buf) {
	const FileRef& file = m_pipe->get_file(); 
	int r = uv_fs_read(loop,&m_req,file->get_file(),buf,1,offset,&FSReq::fs_req_cb);
	if (r >= 0) {
		add_ref();
	}
	return r;
}


void FSReadPipeReq::on_cb() {
	m_pipe->on_read(*this);
}

int FSReadPipe::start_read(uv_loop_t* loop) {
	FSReadPipeReqRef req(new FSReadPipeReq(FSReadPipeRef(this)));
	uv_buf_t* buf = alloc_buffer();
	int r = req->read(loop,m_pos,buf);
	return r;
}



int FileOpenReq::open(lua_State* L,const char* path,int flags,int mode,const luabind::function& f) {
	m_cb.assign(L,f);
	int r = uv_fs_open(llae_get_loop(L),&m_req,path,flags,mode,&FSReq::fs_req_cb);
	if (r < 0) {
		m_cb.reset(L);
		return r;
	}
	add_ref();
	return r;
}

void FileOpenReq::on_result(lua_State* L,const char* err) {
	if (err) {
		FSLuaReq::on_result(L,err);
		return;
	}
	FileRef file(new File(m_req.result,m_req.loop));
	m_cb.callv(L,"FileOpenReq::on_result",file,err);
}



FileSendPipe::FileSendPipe(const FileRef& file,const StreamRef& stream) : FSReadPipe(file,-1,0),m_stream(stream) {
	//std::cout << "FileSendPipe::FileSendPipe" << std::endl;
}

FileSendPipe::~FileSendPipe() {
	//std::cout << "FileSendPipe::~FileSendPipe" << std::endl;
}

uv_buf_t* FileSendPipe::alloc_buffer() {
	m_write = MemWriteReqRef(new MemWriteReq());
	return m_write->alloc(BUFFER_SIZE);
}

int FileSendPipe::send(lua_State* L,const luabind::function& f) {
	m_cb.assign(L,f);
	int r = start_read(llae_get_loop(L));
	if (r < 0) {
		m_cb.reset(L);
		return r;
	}
	add_ref();
	return r;
}

int FileSendPipe::on_data(uv_loop_t* loop,int64_t size) {
	if (m_write) {
		m_write->resize(size);
		int res = m_write->write(m_stream->get_stream());
		if (res < 0) {
			return res;
		}
	}
	return 0;
}


File::File(uv_file f,uv_loop_t* loop) : m_file(f),m_loop(loop) {

}


void File::on_release() {
	if (m_file < 0) {
		delete this;
	} else {
		uv_file f = m_file;
		FileCloseReq* close = new FileCloseReq(Ref<File>(this));
		int r = close->close(m_loop,f);
		if (r<0) {
			std::cerr << "failed close file " <<  uv_strerror(r) << std::endl;
		} else {
			m_file = -1;
		}
	}
}


static const char* file_mt = "llae.File";

void File::push(lua_State* L) {
	/*FileRef* ref = */new (static_cast<FileRef*>(lua_newuserdata(L,sizeof(FileRef)))) FileRef(this);
	luaL_setmetatable(L,file_mt);
}

int File::open(lua_State* L) {
	const char* path = luaL_checkstring(L,1);
	luabind::function f = luabind::S<luabind::function>::get(L,2);
	int flags = luaL_optinteger(L,3,0);
	int mode = luaL_optinteger(L,4,UV_FS_O_RDONLY);
	FileOpenReqRef req(new FileOpenReq());
	int res = req->open(L,path,flags,mode,f);
	lua_llae_handle_error(L,"File::open",res);
	return 0;
}

void File::send(lua_State* L,const StreamRef& stream, const luabind::function& f) {
	FileSendPipeRef pipe(new FileSendPipe( FileRef(this) , stream ));
	int res = pipe->send(L,f);
	lua_llae_handle_error(L,"File::send",res);
}

void File::lbind(lua_State* L) {
	luaL_newmetatable(L,file_mt);
	lua_newtable(L);
	luabind::bind(L,"send",&File::send);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&FileRef::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}


extern "C" int luaopen_llae_file(lua_State* L) {
    
    luaL_Reg reg[] = {
        { "stat", File::stat },
        { "open", File::open },
        { "mkdir", File::mkdir },
        { "scandir", File::scandir },
        { NULL, NULL }
    };
    lua_newtable(L);
    luaL_setfuncs(L, reg, 0);
    return 1;
}

// FileSendReq::FileSendReq(const StreamRef& stream) : m_stream(stream) {
// 	m_uv_buf.base = m_buf;
// 	m_uv_buf.len = BUFFER_SIZE;
// }

// void FileSendReq::on_open() {
// 	FileOpReq::on_open();
// 	if (m_file < 0) {
// 		return;
// 	}
// 	int r = uv_fs_read()
// }

// int FileSendReq::send(lua_State* L,const char* path,const luabind::function& f) {
// 	int r = open(llae_get_loop(L),path,0,UV_FS_O_RDONLY);
// 	if (r < 0) {
// 		return r;
// 	} else {
// 		m_cb.assign(f);
// 	}
// 	return r;
// } 

// int File::send(lua_State* L) {
// 	const char* path = luaL_checkstring(L,1);
// 	StreamRef stream = luabind::S<StreamRef>::get(L,2);
// 	luabind::function f = luabind::S<luabind::function>::get(L,3);
// 	FileSendReq req(new FileSendReq(stream));
// 	int res = req->send(L,path,f);
// 	lua_llae_handle_error(L,"File::send",res);
// 	return 0;
// }
