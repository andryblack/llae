#ifndef __LLAE_TLS_H_INCLUDED__
#define __LLAE_TLS_H_INCLUDED__

#include <uv.h>
#include "uv_req_holder.h"
#include "lua_holder.h"
#include "luabind.h"
#include "stream.h"
#include <list>
#include <openssl/ssl.h>
#include "work.h"
#include "async.h"
#include <mutex>

class TLSCtx : public RefCounter {
private:
	SSL_CTX* m_ctx;
public:
	TLSCtx();
	~TLSCtx();
	SSL_CTX* get_ctx() { return m_ctx; }

	void push(lua_State* L);
	static int lnew(lua_State* L);
	static void lbind(lua_State* L);
};
typedef Ref<TLSCtx> TLSCtxRef;


class TLS : public RefCounter {
private:
	TLSCtxRef m_ctx;
	SSL* m_ssl;
	static BIO_METHOD* bio_s_tls();
	static int bio_tls_write(BIO *bio, const char *buf, int num);
	static int bio_tls_read(BIO *bio, char *buf, int size);
	static int bio_tls_puts(BIO *bio, const char *str);
	static long bio_tls_ctrl(BIO *bio, int cmd, long num, void *ptr);
	static int bio_tls_create(BIO* bio);

	StreamRef m_stream;
	void release_stream();
	int start_read();
	
	int bio_read(char *buf, int size);
	int bio_write(const char *buf, int size);
	int bio_eof();
	BIO* m_stream_bio;

	void stream_alloc_cb(size_t suggested_size, uv_buf_t* buf);
	void stream_read_cb(ssize_t nread, const uv_buf_t* buf);
	

	class StartReadAsync : public AsyncBase {
	private:
		TLS* m_tls;
	public:
		StartReadAsync(uv_loop_t* loop) : AsyncBase(loop),m_tls(0) {}
		void set_tls(TLS* tls) { m_tls = tls; }
		virtual void on_async() {
			if (m_tls) {
				m_tls->start_read();
			}
		}
	};
	Ref<StartReadAsync> m_start_read_async;

	std::mutex m_read_mutex;
	std::condition_variable m_read_cond;

	struct read_buffer {
		std::vector<char> data;
		size_t pos;
		size_t size;
		void put(void* data,size_t size);
	};
	std::list<read_buffer> m_readed_buf;

	class ReadReq : public ReadRequest {
	protected:
		Ref<TLS> m_tls;
	public:
		ReadReq(const Ref<TLS>& tls) : m_tls(tls) {}
		virtual void on_alloc(Stream* s,size_t suggested_size, uv_buf_t* buf) {
			ReadRequest::on_alloc(s,suggested_size,buf);
			m_tls->stream_alloc_cb(suggested_size,buf);
		}
		virtual void on_read(Stream* s,ssize_t nread, const uv_buf_t* buf) {
			m_tls->stream_read_cb(nread,buf);
			ReadRequest::on_read(s,nread,buf);
		}
	};
	typedef Ref<TLS> TLSRef;
	class TLSWork : public ThreadWorkReq{
	protected:
		TLSRef m_tls;
		int m_result;
		bool process_ssl_result(int r);
		virtual int finish(lua_State* L);
	public:
		TLSWork(const TLSRef& tls) : m_tls(tls),m_result(SSL_ERROR_NONE) {}
		virtual void on_after_work(int status);
	};
	typedef Ref<TLSWork> WorkWaitRef;
	class HandshakeWork : public TLSWork {
	public:
		HandshakeWork(const TLSRef& tls) : TLSWork(tls) {}
		bool process(TLS* tls);
		virtual void on_work();
		
	};
	typedef Ref<HandshakeWork> HandshakeWorkRef;

	class WriteWork : public TLSWork {
	private:
		std::vector<uv_buf_t> m_buffers;
		std::vector<LuaHolder> m_data;
	public:
		WriteWork(const TLSRef& tls) : TLSWork(tls) {}
		// bool process(TLS* tls);
		// int finish(lua_State* L);
		virtual void on_work();
		// virtual void on_after_work(int status);
		int start(lua_State* L);
	};
	typedef Ref<WriteWork> WriteWorkRef;

	class ReadWork : public TLSWork {
	private:
		std::vector<char> m_buffer;
		virtual int finish(lua_State* L);
	public:
		ReadWork(const TLSRef& tls) : TLSWork(tls) {}
		virtual void on_work();
	};
	typedef Ref<ReadWork> ReadWorkRef;

public:
	TLS(const TLSCtxRef& ctx,const StreamRef& stream);
	~TLS();

	void push(lua_State* L);
	static int lnew(lua_State* L);
	static void lbind(lua_State* L);

	SSL* get_ssl() { return m_ssl; }
	void handshake(lua_State* L);
	void write(lua_State* L);
	void read(lua_State* L);
	void close(lua_State* L);
};
typedef Ref<TLS> TLSRef;

// class TLSWork : public ThreadWorkReq {
// protected:
// 	TLSRef m_tls;
// 	int m_result;
// public:
// 	TLSWork(const TLSRef& tls) : m_tls(tls),m_result(0) {}
// };

// class TLSHandshakeWork : public TLSWork {
// protected:
// 	virtual void on_work();
// 	virtual void on_after_work(int status);
// public:
// 	TLSHandshakeWork(const TLSRef& tls) : TLSWork(tls) {}
// };


#endif /*__LLAE_TLS_H_INCLUDED__*/