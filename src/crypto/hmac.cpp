#include "hmac.h"
#include <memory>
#include "crypto.h"
#include "uv/work.h"
#include "uv/buffer.h"
#include "llae/app.h"
#include "common/intrusive_ptr.h"
#include "lua/bind.h"
#include "lua/stack.h"
#include "uv/luv.h"

META_OBJECT_INFO(crypto::hmac,meta::object)

namespace crypto {


	hmac::hmac(const mbedtls_md_info_t* info) {
		mbedtls_md_init(&m_ctx);
		mbedtls_md_setup(&m_ctx,info,1);
	}

	hmac::~hmac() {
		mbedtls_md_free(&m_ctx);
	}

	using hmac_ptr = common::intrusive_ptr<hmac>;
	class hmac::async : public uv::work {
	protected:
		hmac_ptr m_hmac;
		int m_status = 0;
	public:
		explicit async(hmac_ptr&& m) : m_hmac(std::move(m)) {}
	};

	class hmac::update_async : public hmac::async {
	private:
		uv::write_buffers m_buffers;
	public:
		explicit update_async(hmac_ptr&& m) : hmac::async(std::move(m)) {}
		virtual void on_work() {
			for (auto& b:m_buffers.get_buffers()) {
				m_status = mbedtls_md_hmac_update(&m_hmac->m_ctx, 
					reinterpret_cast<const unsigned char*>(b.base), b.len );
				if (m_status != 0)
					break;
			}
		}
		bool put(lua::state& l) {
			return m_buffers.put(l);
		}
		virtual void on_after_work(int status) {
            if (llae::app::closed(get_loop())) {
                m_buffers.release();
                m_hmac->release();
            } else {
                uv::loop loop(get_loop());
                lua::state& l(llae::app::get(loop).lua());
                m_buffers.reset(l);
                m_hmac->on_update_completed(l,status,m_status);
            }
		}
	};

	class hmac::start_async : public hmac::async {
	private:
		uv::buffer_ptr m_key;
	public:
		explicit start_async(hmac_ptr&& m) : hmac::async(std::move(m)) {}
		virtual void on_work() {
			m_status = mbedtls_md_hmac_starts(&m_hmac->m_ctx,
				reinterpret_cast<const unsigned char*>(m_key->get_base()),m_key->get_len());
			m_key.reset();
		}
		bool put(lua::state& l) {
			m_key = uv::buffer::get(l,-1);
	        return m_key;
		}
		virtual void on_after_work(int status) {
            if (!llae::app::closed(get_loop())) {
                uv::loop loop(get_loop());
                lua::state& l(llae::app::get(loop).lua());
                m_hmac->on_start_completed(l,status,m_status);
            }
		}
	};

	class hmac::finish_async : public hmac::async {
	private:
		uv::buffer_ptr m_digest;
	public:
		explicit finish_async(hmac_ptr&& m) : hmac::async(std::move(m)) {}
		virtual void on_work() {
			size_t size = mbedtls_md_get_size(m_hmac->m_ctx.md_info);
			m_digest = uv::buffer::alloc(size);
			m_status = mbedtls_md_hmac_finish(&m_hmac->m_ctx,static_cast<unsigned char*>(m_digest->get_base()));
		}
		virtual void on_after_work(int status) {
			uv::loop loop(get_loop());
			m_hmac->on_finish_completed(loop,status,m_status,std::move(m_digest));
		}
	};


	lua::multiret hmac::start(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("hmac::start is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("hmac::start operation in progress");
			return {2};
		}
		{
			common::intrusive_ptr<start_async> req{new start_async(hmac_ptr(this))};
			l.pushvalue(2);
			if (!req->put(l)) {
				l.pushnil();
				l.pushstring("md::start invalid data");
				return {2};
			}

			l.pushthread();
			m_cont.set(l);
			
			int r = req->queue_work(llae::app::get(l).loop());
			if (r < 0) {
				m_cont.reset(l);
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}

	lua::multiret hmac::reset(lua::state& l) {
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("hmac::reset operation in progress");
			return {2};
		}
		if (!m_started) {
			l.pushnil();
			l.pushstring("hmac::reset not started");
			return {2};
		}
		auto mbedlsstatus = mbedtls_md_hmac_reset(&m_ctx);

		int nres = 1;
		if (mbedlsstatus!=0) {
            l.pushnil();
            push_error(l,"reset failed, code:%d, %s",mbedlsstatus);
            nres = 2;
        } else {
            l.pushboolean(true);
            nres = 1;
        }
        return {nres};
	}

	lua::multiret hmac::update(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("hmac::update is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("hmac::update operation in progress");
			return {2};
		}
		if (!m_started) {
			l.pushnil();
			l.pushstring("hmac::update not started");
			return {2};
		}
		{
			common::intrusive_ptr<update_async> req{new update_async(hmac_ptr(this))};
			l.pushvalue(2);
			if (!req->put(l)) {
				l.pushnil();
				l.pushstring("md::update invalid data");
				return {2};
			}

			l.pushthread();
			m_cont.set(l);
			
			int r = req->queue_work(llae::app::get(l).loop());
			if (r < 0) {
				m_cont.reset(l);
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}

	lua::multiret hmac::finish(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("hmac::finish is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("hmac::finish operation in progress");
			return {2};
		}
		{
			l.pushthread();
			m_cont.set(l);
			common::intrusive_ptr<finish_async> req{new finish_async(hmac_ptr(this))};
			
			int r = req->queue_work(llae::app::get(l).loop());
			if (r < 0) {
				m_cont.reset(l);
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}

	void hmac::on_start_completed(lua::state& l,int uvstatus,int mbedlsstatus) {
		if (!m_cont.valid())
			return;
		
		m_cont.push(l);
		m_cont.reset(l);
		auto toth = l.tothread(-1);
		toth.checkstack(3);
        
        int nres = 0;
        if (uvstatus<0) {
            toth.pushnil();
            uv::push_error(toth,uvstatus);
            nres = 2;
        } else if (mbedlsstatus!=0) {
            toth.pushnil();
            push_error(toth,"update failed, code:%d, %s",mbedlsstatus);
            nres = 2;
        } else {
            toth.pushboolean(true);
            nres = 1;
            m_started = true;
        }
        
		auto s = toth.resume(l,nres);
		if (s != lua::status::ok && s != lua::status::yield) {
			llae::app::show_error(toth,s);
		}
		l.pop(1);// thread
	}

	void hmac::on_update_completed(lua::state& l,int uvstatus,int mbedlsstatus) {
		if (!m_cont.valid())
			return;
		
		m_cont.push(l);
		m_cont.reset(l);
		auto toth = l.tothread(-1);
		toth.checkstack(3);
        
        int nres = 0;
        if (uvstatus<0) {
            toth.pushnil();
            uv::push_error(toth,uvstatus);
            nres = 2;
        } else if (mbedlsstatus!=0) {
            toth.pushnil();
            push_error(toth,"update failed, code:%d, %s",mbedlsstatus);
            nres = 2;
        } else {
            toth.pushboolean(true);
            nres = 1;
        }
        
		auto s = toth.resume(l,nres);
		if (s != lua::status::ok && s != lua::status::yield) {
			llae::app::show_error(toth,s);
		}
		l.pop(1);// thread
	}
	void hmac::on_finish_completed(uv::loop& loop,int uvstatus,int mbedlsstatus,uv::buffer_ptr&& digest) {
		if (!m_cont.valid())
			return;
		lua::state& l(llae::app::get(loop).lua());
		
		m_cont.push(l);
		m_cont.reset(l);
		m_started = false;
		auto toth = l.tothread(-1);
		toth.checkstack(3);
        
        int nres = 0;
        if (uvstatus<0) {
            toth.pushnil();
            uv::push_error(toth,uvstatus);
            nres = 2;
        } else if (mbedlsstatus!=0) {
            toth.pushnil();
            push_error(toth,"update failed, code:%d, %s",mbedlsstatus);
            nres = 2;
        } else {
            lua::push(toth,std::move(digest));
            nres = 1;
        }
		auto s = toth.resume(l,nres);
		if (s != lua::status::ok && s != lua::status::yield) {
			llae::app::show_error(toth,s);
		}
		l.pop(1);// thread
	}

	void hmac::lbind(lua::state& l) {
		lua::bind::function(l,"new",&hmac::lnew);
		lua::bind::function(l,"start",&hmac::start);
		lua::bind::function(l,"reset",&hmac::reset);
		lua::bind::function(l,"update",&hmac::update);
		lua::bind::function(l,"finish",&hmac::finish);
	}

	lua::multiret hmac::lnew(lua::state& l) {
		const char* alg = l.checkstring(1);
		auto info = mbedtls_md_info_from_string(alg);
		if (!info) {
			l.pushnil();
			l.pushfstring("unknown hmac algorithm '%s'",alg);
			return {2};
		}
		lua::push(l,hmac_ptr(new hmac(info)));
		return {1};
	}
}
