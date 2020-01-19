#include "stream.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "meta/info.h"

META_INFO(Stream,void)

WriteReqBase::WriteReqBase() {
	attach();
}

uv_req_t* WriteReqBase::get_req() {
	return reinterpret_cast<uv_req_t*>(&m_write);
}

void WriteReqBase::on_end(int status) {

}

void WriteReqBase::write_cb(uv_write_t* req, int status) {
	WriteReqBase* r = static_cast<WriteReqBase*>(req->data);
	r->on_end(status);
	r->remove_ref();
}


int WriteReqBase::write(uv_stream_t* stream) {
	int res = uv_write(&m_write,stream,get_buffers(),get_buffers_count(),&WriteReqBase::write_cb);
	if (res >= 0) {
		add_ref();
	}
	return res;
}

WriteReq::WriteReq() {

}


void WriteReq::on_end(int status) {
	lua_State* L = llae_get_vm(m_write.handle);
	for (std::vector<LuaHolder>::iterator it = m_data.begin();it!=m_data.end();++it) {
		it->reset(L);
	}
}



void WriteReq::put(lua_State* L,int idx) {
	int t = lua_type(L,idx);
	if ( t == LUA_TTABLE) {
		size_t l = lua_rawlen(L,idx);
		size_t i = m_buffers.size();
		m_buffers.resize(i+l);
		m_data.resize(m_buffers.size());
		for (size_t j=0;j<l;++j) {
			lua_rawgeti(L,idx,j+1);
			lua_pushvalue(L,-1);
			m_data[i].assign(L);
			luabind::S<uv_buf_t>::get(L,-1,&m_buffers[i]);
			lua_pop(L,1);
			++i;
		}
	} else {
		size_t i = m_buffers.size();
		m_buffers.resize(m_buffers.size()+1);
		m_data.resize(m_buffers.size());
		lua_pushvalue(L,idx);
		luabind::S<uv_buf_t>::get(L,-1,&m_buffers[i]);
		m_data[i].assign(L);
	}
}

MemWriteReq::MemWriteReq() {
}


uv_buf_t* MemWriteReq::alloc(size_t size) {
	resize(size);
	return &m_buf;
}

void MemWriteReq::resize(size_t size) {
	m_data.resize(size);
	m_buf.base = m_data.data();
	m_buf.len = m_data.size();
}





ShootdownReq::ShootdownReq() {
	attach();
}

uv_req_t* ShootdownReq::get_req() {
	return reinterpret_cast<uv_req_t*>(&m_req);
}

void ShootdownReq::shutdown_cb(uv_shutdown_t* req, int status) {
	ShootdownReq* r = static_cast<ShootdownReq*>(req->data);
	r->on_end(status);
	r->remove_ref();
}
void ShootdownReq::on_end(int status) {
	m_stream->close();
}
int ShootdownReq::start(const StreamRef& stream) {
	m_stream = stream;
	int res = uv_shutdown(&m_req,stream->get_stream(),&ShootdownReq::shutdown_cb);
	if (res>=0) {
		add_ref();
	}
	return res;
}

Stream::Stream() {

}

void Stream::on_gc(lua_State *L) {
	//m_read_th.reset(L);
}

const char* Stream::get_mt_name() {
	return "llae.Stream";
}

uv_handle_t* Stream::get_handle() {
	return reinterpret_cast<uv_handle_t*>(get_stream());
}

void Stream::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	static_cast<Stream*>(handle->data)->on_alloc(suggested_size,buf);
}

void Stream::read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	static_cast<Stream*>(stream->data)->on_read(nread,buf);
}

void ReadRequest::on_alloc(Stream* s,size_t suggested_size, uv_buf_t* buf) {
	buf->base = static_cast<char*>(malloc(suggested_size));
  	buf->len = suggested_size;
}
void ReadRequest::on_read(Stream* s,ssize_t nread, const uv_buf_t* buf) {
	if (buf->base) {
		free(buf->base);
	}
}

void Stream::on_alloc(size_t suggested_size, uv_buf_t* buf) {
	if (m_read_req) {
		m_read_req->on_alloc(this,suggested_size,buf);
	}
}

void Stream::on_read(ssize_t nread, const uv_buf_t* buf) {
	const char* err = 0;
	uv_read_stop(get_stream());
	if (m_read_req) {
		ReadRequestRef req(m_read_req);
		m_read_req.reset();
		req->on_read(this,nread,buf);
	}
}

void Stream::write(lua_State* L) {
	WriteReqRef req(new WriteReq());
	req->put(L,2);
	int res = req->write(get_stream());
	lua_llae_handle_error(L,"Stream::write",res);
}

int Stream::start_stream_read(const ReadRequestRef& req) {
	m_read_req = req;
	int r = uv_read_start(get_stream(),&Stream::alloc_cb,&Stream::read_cb);
	if (r < 0) {
		m_read_req.reset();
	}
	return r;
}

void ThreadReadRequest::start(lua_State* L) {
	m_read_th.assign(L);
}
void ThreadReadRequest::on_read(Stream* s,ssize_t nread, const uv_buf_t* buf) {
	lua_State* L = llae_get_vm(s->get_stream());
	if (L && m_read_th) {
		if (nread > 0) {
			const uv_buf_t b = uv_buf_init(buf->base, nread);
			m_read_th.resumev(L,"Stream::on_read",&b,luabind::lnil());
		} else if (nread == UV_EOF) {
			m_read_th.resumev(L,"Stream::on_read",luabind::lnil(),luabind::lnil());
			m_read_th.reset(L);
		} else if (nread < 0) {
			const char* err = uv_strerror(nread);
			m_read_th.resumev(L,"Stream::on_read",luabind::lnil(),err);
			m_read_th.reset(L);
		} else { // == 0
			
		} 
	}
	ReadRequest::on_read(s,nread,buf);
}


int Stream::read(lua_State* L) {
	Stream* self = StreamRef::get_ptr(L,1);
	if (!self->m_read_req) {
		if (lua_isyieldable(L)) {
			lua_pushthread(L);
			ThreadReadRequest* r = new ThreadReadRequest();
			ReadRequestRef req(r);
			r->start(L);
			int res = self->start_stream_read(req);
			if (res < 0) {
				lua_llae_handle_error(L,"Stream::read",res);
			}
			lua_yield(L,0);
		} else {
			luaL_error(L,"Stream::read sync not supported");
		}
	} else {
		luaL_error(L,"Stream::read already started");
	}
	
	//printf("read %p\n",L);
	//
	return 0;
}

void Stream::shutdown(lua_State* L) {
	ShootdownReqPtr req(new ShootdownReq());
	int res = req->start(StreamRef(this));
	lua_llae_handle_error(L,"Stream::shutdown",res);
}

void Stream::close() {
	m_read_req.reset();
	UVHandleHolder::close();
}

void Stream::lbind(lua_State* L) {
	luaL_newmetatable(L,get_mt_name());
	lua_newtable(L);
	luabind::bind(L,"read",&Stream::read);
	luabind::bind(L,"write",&Stream::write);
	luabind::bind(L,"shutdown",&Stream::shutdown);
	luabind::bind(L,"close",&Stream::close);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&UVHandleHolder::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}
