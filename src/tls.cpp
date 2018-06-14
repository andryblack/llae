#include "tls.h"

#include "stream.h"
#include <openssl/bio.h>
#include <openssl/err.h>

#include <iostream>
#include <cassert>

#if (OPENSSL_VERSION_NUMBER<0x10100000L)
	static void BIO_set_init(BIO *a, int init) {
		a->init = init;
	}
	static int BIO_get_shutdown(BIO *a) {
		return a->shutdown;
	}
	static void BIO_set_shutdown(BIO *a, int shut) {
		a->shutdown = shut;
	}
	static void BIO_set_data(BIO *a, void *ptr) {
		a->ptr = ptr;
	}
	static void *BIO_get_data(BIO *a) {
		return a->ptr;
	}
#endif
#if (OPENSSL_VERSION_NUMBER<0x00906000L)
	static BIO *BIO_next(BIO *b) {
		return b->next_bio;
	}
#endif

static const char* tls_ctx_mt = "llae.tls.ctx";
static const char* tls_mt = "llae.tls";

static BIO* bio_err = 0;
static void push_ssl_error(lua_State* L,int err) {

	switch (err) {
		case SSL_ERROR_NONE: lua_pushstring(L,"SSL_ERROR_NONE"); break;
		case SSL_ERROR_ZERO_RETURN: lua_pushstring(L,"SSL_ERROR_ZERO_RETURN"); break;
		case SSL_ERROR_WANT_READ: lua_pushstring(L,"SSL_ERROR_WANT_READ"); break;
		case SSL_ERROR_WANT_WRITE: lua_pushstring(L,"SSL_ERROR_WANT_WRITE"); break;
		case SSL_ERROR_WANT_CONNECT: lua_pushstring(L,"SSL_ERROR_WANT_CONNECT"); break;
		case SSL_ERROR_WANT_ACCEPT: lua_pushstring(L,"SSL_ERROR_WANT_ACCEPT"); break;
		case SSL_ERROR_WANT_X509_LOOKUP: lua_pushstring(L,"SSL_ERROR_WANT_X509_LOOKUP"); break;
		case SSL_ERROR_SYSCALL: lua_pushstring(L,"SSL_ERROR_SYSCALL"); break;
		case SSL_ERROR_SSL: lua_pushstring(L,"SSL_ERROR_SSL"); break;
		default:
			lua_pushnumber(L,err);
			break;
	}

}

TLSCtx::TLSCtx() {
	m_ctx = SSL_CTX_new(SSLv23_method());
}

TLSCtx::~TLSCtx() {
	SSL_CTX_free(m_ctx);
}

int TLSCtx::lnew(lua_State* L) {
	(new TLSCtx())->push(L);
	return 1;
}
void TLSCtx::push(lua_State* L) {
	/*FileRef* ref = */new (lua_newuserdata(L,sizeof(TLSCtxRef))) TLSCtxRef(this);
	luaL_setmetatable(L,tls_ctx_mt);
}
void TLSCtx::lbind(lua_State* L) {
	SSL_library_init();
	SSL_load_error_strings();

	bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);

	luaL_newmetatable(L,tls_ctx_mt);
	lua_newtable(L);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&TLSCtxRef::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}




BIO_METHOD * TLS::bio_s_tls() {
#if (OPENSSL_VERSION_NUMBER<0x10100000L)
	static BIO_METHOD bio_methods;
	static bool need_init = true;
	if (need_init) {
		need_init = false;
		memset(&bio_methods,0,sizeof(bio_methods));
		bio_methods.type = BIO_TYPE_MEM;
		bio_methods.name = "tls_callbacks";
		bio_methods.bwrite = &TLS::bio_tls_write;
		bio_methods.bread = &TLS::bio_tls_read;
		bio_methods.bputs = &TLS::bio_tls_puts;
		bio_methods.ctrl = &TLS::bio_tls_ctrl;
		bio_methods.create = &TLS::bio_tls_create;
	}
	return &bio_methods;
#else
	static BIO_METHOD* bio_methods = 0;
	if (!bio_methods) {
		bio_methods = BIO_meth_new(BIO_TYPE_MEM,"tls_callbacks");
		BIO_meth_set_write(bio_methods,&TLS::bio_tls_write);
		BIO_meth_set_read(bio_methods,&TLS::bio_tls_read);
		BIO_meth_set_puts(bio_methods,&TLS::bio_tls_puts);
		BIO_meth_set_ctrl(bio_methods,&TLS::bio_tls_ctrl);
		BIO_meth_set_create(bio_methods,&TLS::bio_tls_create);
	}
	return bio_methods;
#endif
}
int TLS::bio_tls_create(BIO* bio) {
	BIO_set_init(bio,1);
	return 1;
}
int TLS::bio_tls_puts(BIO *bio, const char *str) {
	return bio_tls_write(bio, str, strlen(str));
}
long TLS::bio_tls_ctrl(BIO *bio, int cmd, long num, void *ptr) {
	long ret = 1;
	TLS *tls = reinterpret_cast<TLS*>(BIO_get_data(bio));
	switch (cmd) {
	case BIO_CTRL_GET_CLOSE:
		ret = (long)BIO_get_shutdown(bio);;
		break;
	case BIO_CTRL_SET_CLOSE:
		BIO_set_shutdown(bio,num);
		break;
	case BIO_CTRL_DUP:
	case BIO_CTRL_FLUSH:
		break;
	case BIO_CTRL_EOF:
		ret = reinterpret_cast<TLS*>(BIO_get_data(bio))->bio_eof();
		break;
	case BIO_CTRL_INFO:
	case BIO_CTRL_GET:
	case BIO_CTRL_SET:
	default:
		ret = BIO_ctrl(BIO_next(bio), cmd, num, ptr);
	}

	return ret;
}
int TLS::bio_tls_write(BIO *bio, const char *buf, int num) {
	TLS *tls = reinterpret_cast<TLS*>(BIO_get_data(bio));
	
	BIO_clear_retry_flags(bio);
	int rv = tls->bio_write(buf,num);
	// if (rv == TLS_WANT_POLLIN) {
	// 	BIO_set_retry_read(bio);
	// 	rv = -1;
	// } else if (rv == TLS_WANT_POLLOUT) {
	// 	BIO_set_retry_write(bio);
	// 	rv = -1;
	// }
	// return (rv);
	return rv;
}

int TLS::bio_tls_read(BIO *bio, char *buf, int size) {
	TLS *tls = reinterpret_cast<TLS*>(BIO_get_data(bio));
	
	BIO_clear_retry_flags(bio);
	int rv = tls->bio_read(buf, size);
	// if (rv == TLS_WANT_POLLIN) {
	// 	BIO_set_retry_read(bio);
	// 	rv = -1;
	// } else if (rv == TLS_WANT_POLLOUT) {
	// 	BIO_set_retry_write(bio);
	// 	rv = -1;
	// }
	return rv;
}


TLS::TLS(const TLSCtxRef& ctx,const StreamRef& stream) : m_ctx(ctx),m_stream(stream) {
	m_start_read_async.reset(new StartReadAsync(m_stream->get_stream()->loop));
	m_start_read_async->set_tls(this);
	m_ssl = SSL_new(m_ctx->get_ctx());
	assert(m_ssl);
	SSL_set_app_data(m_ssl,reinterpret_cast<char*>(this));
	m_stream_bio = BIO_new(bio_s_tls());
	assert(m_stream_bio);
	BIO_set_data(m_stream_bio,this);
	SSL_set_bio(m_ssl,m_stream_bio,m_stream_bio);
	SSL_set_connect_state(m_ssl);
}

TLS::~TLS() {
	release_stream();
	SSL_free(m_ssl);
	// cleared by ssl
	//BIO_free(m_stream_bio);
}
void TLS::release_stream() {
	if (m_stream) {
		m_stream->close();
		m_stream.reset();
	}
	if (m_start_read_async) {
		m_start_read_async->set_tls(0);
		m_start_read_async.reset();
	}
}

void TLS::stream_alloc_cb(size_t suggested_size, uv_buf_t* buf) {
	// buf->base = static_cast<char*>(malloc(suggested_size));
	// buf->len = suggested_size;
}
void TLS::stream_read_cb(ssize_t nread, const uv_buf_t* buf) {
	//std::cout << "stream_read_cb: " << nread << std::endl;
	MutexLock l(m_read_mutex);
	if (nread>0) {
		m_readed_buf.push_back(read_buffer());
		read_buffer& b(m_readed_buf.back());
		b.put(buf->base,nread);
	} else if (nread == UV_EOF) {
		//std::cout << "stream_read_cb EOF" << std::endl;
		release_stream();
	} else if (nread < 0) {
		std::cout << "error: " << uv_strerror(nread) << std::endl;
		release_stream();
	}
	m_read_cond.signal();
	// if (m_work) {
	// 	if (m_work->resume_on_read(llae_get_vm(m_stream->get_stream()),this)) {
	// 		m_work.reset();
	// 	}
	// }
}

int TLS::start_read() {
	if (!m_stream) {
		return -1;
	}
	//std::cout << "start_read" << std::endl;
	return m_stream->start_stream_read(ReadRequestRef(new ReadReq(TLSRef(this))));
}

void TLS::read_buffer::put(void* d,size_t size) {
	data.reserve(size);
	data.resize(size);
	memcpy(data.data(),d,size);
	pos = 0;
	this->size = size;
}
int TLS::bio_eof() {
	MutexLock l(m_read_mutex);
	int ret = (!m_stream && m_readed_buf.empty()) ? 1: 0;
	std::cout << "bio_eof: " << ret << std::endl;
	return ret;
}
int TLS::bio_read(char *buf, int size) {
	//std::cout << "TLS::read " << size << std::endl;
	MutexLock l(m_read_mutex);
	
	while (m_readed_buf.empty()) {
		if (!m_start_read_async) {
			return -1;
		}
		BIO_set_retry_read(m_stream_bio);
		m_start_read_async->async_send();
		m_read_cond.wait(m_read_mutex);
	}
	int nread = 0;
	while(size > 0) {
		if (m_readed_buf.empty())
			break;
		read_buffer& b(m_readed_buf.front());
		int n = size;
		if (n > b.size) {
			n = b.size;
		}
		memcpy(buf,&b.data[b.pos],n);
		buf = buf + n;
		b.pos += n;
		b.size -= n;
		size -= n;
		nread += n;
		//std::cout << "add " << n << std::endl;
		if (b.size == 0) {
			m_readed_buf.pop_front();
		}
	}
	//std::cout << "readed " << nread << std::endl;
	return nread;
}
int TLS::bio_write(const char *buf, int size) {
	//std::cout << "TLS::write " << size << std::endl;
	if (!m_stream) {
		return -1;
	}
	MemWriteReqRef req(new MemWriteReq());
	uv_buf_t* uvbuf = req->alloc(size);
	memcpy(uvbuf->base,buf,size);
	int r = req->write(m_stream->get_stream());
	if (r < 0) {
		return -1;
	}
	return size;
}

void TLS::handshake(lua_State* L) {
	if (lua_isyieldable(L)) {
		lua_pushthread(L);
		HandshakeWorkRef work(new HandshakeWork(TLSRef(this)));
		int ret = work->start(L);
		if (ret < 0) {
			lua_llae_handle_error(L,"TLS::handshake",ret);
		}
		lua_yield(L,0);
	} else {
		luaL_error(L,"TLS::handshake sync not supported");
	}
}

void TLS::write(lua_State* L) {
	if (lua_isyieldable(L)) {
		//lua_pushthread(L);
		WriteWorkRef work(new WriteWork(TLSRef(this)));
		int ret = work->start(L);
		if (ret < 0) {
			lua_llae_handle_error(L,"TLS::write",ret);
		}
		lua_yield(L,0);
	} else {
		luaL_error(L,"TLS::write sync not supported");
	}
}
void TLS::read(lua_State* L) {
	if (lua_isyieldable(L)) {
		lua_pushthread(L);
		ReadWorkRef work(new ReadWork(TLSRef(this)));
		int ret = work->start(L);
		if (ret < 0) {
			lua_llae_handle_error(L,"TLS::read",ret);
		}
		lua_yield(L,0);
	} else {
		luaL_error(L,"TLS::read sync not supported");
	}
}
void TLS::close(lua_State* L) {
	MutexLock l(m_read_mutex);
	release_stream();
}

bool TLS::TLSWork::process_ssl_result(int ret) {
	//std::cout << "process_ssl_result: " << ret << std::endl;
	if (ret <= 0) {
		m_result = SSL_get_error(m_tls->get_ssl(),ret);
		ERR_print_errors(bio_err);
	} else {
		m_result = SSL_ERROR_NONE;
	}
	return (m_result == SSL_ERROR_NONE) ||  (m_result != SSL_ERROR_WANT_READ);
}

void TLS::TLSWork::on_after_work(int status) {
	if (status != 0) {
		ThreadWorkReq::on_after_work(status);
	} else {
		lua_State* L = llae_get_vm(m_work.loop);
		if (L) {
			if (m_th) {
				int res = finish(L);
				m_th.resumevi(L,"TLSWork::on_after_work",res);
				m_th.reset(L);
			}
		}
	}
}

int TLS::TLSWork::finish(lua_State* L) {
	lua_pushboolean(L,m_result == SSL_ERROR_NONE);
	int r = 1;
	if (m_result != SSL_ERROR_NONE) {
		push_ssl_error(L,m_result);
		r = 2;
	}
	return r;
}

int TLS::WriteWork::start(lua_State* L) {
	int idx = 2;
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

	lua_pushthread(L);
	return TLSWork::start(L);
}

void TLS::WriteWork::on_work() {
	while (!m_buffers.empty()) {
		uv_buf_t& buf(m_buffers.front());
		while (buf.len) {
			int r = SSL_write(m_tls->get_ssl(),buf.base,buf.len);
			if (r > 0) {
				buf.len -= r;
				buf.base += r;
			} else {
				if (process_ssl_result(r)) {
					return;
				}
				break;
			}
		}
		if (buf.len == 0) {
			m_buffers.erase(m_buffers.begin());
		}
	}
}


void TLS::ReadWork::on_work() {
	while (true) {
		m_buffer.resize(1024);
		int r = SSL_read(m_tls->get_ssl(),
			m_buffer.data(),
			m_buffer.size());
		//std::cout << "SSL_read: " << r << std::endl;
		if (r <= 0) {
			if (process_ssl_result(r)) {
				return;
			}
		} else {
			m_result = SSL_ERROR_NONE;
			m_buffer.resize(r);
			break;
		}
	}
}

int TLS::ReadWork::finish(lua_State* L) {
	if (m_result == SSL_ERROR_NONE) {
		lua_pushlstring(L,m_buffer.data(),m_buffer.size());
	} else {
		lua_pushboolean(L,0);
	}
	int r = 1;
	if (m_result != SSL_ERROR_NONE) {
		push_ssl_error(L,m_result);
		r = 2;
	}
	return r;
}

void TLS::HandshakeWork::on_work() {
	while (!process(m_tls.get())) {

	}
}


bool TLS::HandshakeWork::process(TLS* tls) {
	int ret = SSL_connect(tls->get_ssl());
	//std::cout << "TLS::HandshakeWork::process: " << ret << std::endl;
	return process_ssl_result(ret);
}


int TLS::lnew(lua_State* L) {
	TLSCtxRef ctx(TLSCtxRef::get_ref(L,1));
	StreamRef stream(StreamRef::get_ref(L,2));
	(new TLS(ctx,stream))->push(L);
	return 1;
}
void TLS::push(lua_State* L) {
	/*FileRef* ref = */new (lua_newuserdata(L,sizeof(TLSRef))) TLSRef(this);
	luaL_setmetatable(L,tls_mt);
}
void TLS::lbind(lua_State* L) {
	luaL_newmetatable(L,tls_mt);
	lua_newtable(L);
	luabind::bind(L,"handshake",&TLS::handshake);
	luabind::bind(L,"write",&TLS::write);
	luabind::bind(L,"read",&TLS::read);
	luabind::bind(L,"close",&TLS::close);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&TLSRef::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}

