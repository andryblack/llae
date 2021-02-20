#include "md.h"
#include <memory>
#include "crypto.h"
#include "uv/work.h"
#include "uv/buffer.h"
#include "llae/app.h"
#include "common/intrusive_ptr.h"
#include "lua/bind.h"
#include "lua/stack.h"
#include "uv/luv.h"

META_OBJECT_INFO(crypto::md,meta::object)

namespace crypto {


	md::md(const mbedtls_md_info_t* info) {
		mbedtls_md_init(&m_ctx);
		mbedtls_md_setup(&m_ctx,info,0);
	}

	md::~md() {
		mbedtls_md_free(&m_ctx);
	}

	using md_ptr = common::intrusive_ptr<md>;
	class md::async : public uv::work {
	protected:
		md_ptr m_md;
		int m_status = 0;
	public:
		explicit async(md_ptr&& m) : m_md(std::move(m)) {}
	};

	class md::update_async : public md::async {
	private:
		uv::write_buffers m_buffers;
	public:
		explicit update_async(md_ptr&& m) : md::async(std::move(m)) {}
		virtual void on_work() {
			for (auto& b:m_buffers.get_buffers()) {
				m_status = mbedtls_md_update(&m_md->m_ctx, 
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
                m_md->release();
            } else {
                uv::loop loop(get_loop());
                lua::state& l(llae::app::get(loop).lua());
                m_buffers.reset(l);
                m_md->on_update_completed(l,status,m_status);
            }
		}
	};

	class md::finish_async : public md::async {
	private:
		uv::buffer_ptr m_digest;
	public:
		explicit finish_async(md_ptr&& m) : md::async(std::move(m)) {}
		virtual void on_work() {
			size_t size = mbedtls_md_get_size(m_md->m_ctx.md_info);
			m_digest = uv::buffer::alloc(size);
			m_status = mbedtls_md_finish(&m_md->m_ctx,static_cast<unsigned char*>(m_digest->get_base()));
		}
		virtual void on_after_work(int status) {
			uv::loop loop(get_loop());
			m_md->on_finish_completed(loop,status,m_status,std::move(m_digest));
		}
	};

	lua::multiret md::update(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("md::update is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("md::update operation in progress");
			return {2};
		}
		if (!m_started) {
			int r = mbedtls_md_starts(&m_ctx);
			if (r!=0) {
				l.pushnil();
				push_error(l,"mbedtls_md_starts failed, code:%d, %s",r);
				return {2};
			}
			m_started = true;
		}
		{
			common::intrusive_ptr<update_async> req{new update_async(md_ptr(this))};
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

	lua::multiret md::finish(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("md::finish is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("md::finish operation in progress");
			return {2};
		}
		{
			l.pushthread();
			m_cont.set(l);
			common::intrusive_ptr<finish_async> req{new finish_async(md_ptr(this))};
			
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

	void md::on_update_completed(lua::state& l,int uvstatus,int mbedlsstatus) {
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
	void md::on_finish_completed(uv::loop& loop,int uvstatus,int mbedlsstatus,uv::buffer_ptr&& digest) {
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

	void md::lbind(lua::state& l) {
		lua::bind::function(l,"new",&md::lnew);
		lua::bind::function(l,"update",&md::update);
		lua::bind::function(l,"finish",&md::finish);
	}

	lua::multiret md::lnew(lua::state& l) {
		const char* alg = l.checkstring(1);
		auto info = mbedtls_md_info_from_string(alg);
		if (!info) {
			l.pushnil();
			l.pushfstring("unknown md algorithm '%s'",alg);
			return {2};
		}
		lua::push(l,md_ptr(new md(info)));
		return {1};
	}
}
