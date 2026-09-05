#include "hmac.h"
#include "crypto.h"
#include "llae/promise.h"
#include "llae/sequental.h"
#include "llae/work.h"
#include "llae/buffer.h"
#include "llae/loop.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "llae/write_buffers.h"
#include "llae/async_bind.h"
#include "md.h"

META_OBJECT_INFO(crypto::hmac,meta::object)

namespace crypto {


	hmac::hmac(const mbedtls_md_info_t* info) : m_info(info) {
		mbedtls_md_init(&m_ctx);
		mbedtls_md_setup(&m_ctx,m_info,1);
	}

	hmac::~hmac() {
		mbedtls_md_free(&m_ctx);
	}

	llae::result<void> hmac::start_impl(const llae::buffer_view& key) {
		auto status = mbedtls_md_hmac_starts(&m_ctx,
			reinterpret_cast<const unsigned char*>(key.get_base()),key.get_len());
		return make_result(status);
	}

	llae::result<void> hmac::sync_start(const llae::buffer_base_ptr& key) {
		if (!key) {
			return llae::string_error::create("need key");
		}
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("start")) {
			return std::move(e);
		}
		return start_impl(*key);
	}
	llae::result_promise_ptr<void> hmac::async_start(llae::loop& a,llae::buffer_base_ptr key) {
		if (!key) {
			return llae::make_result_promise_string_error<void>("need key");
		}
		using work_t = llae::sequental_method_work<void, hmac>;
		return work_t::start(a,this,&hmac::m_seq,"start", [lkey = std::move(key)](hmac& self) {
			return self.start_impl(*lkey);
		});
	}

	llae::result<void> hmac::update_impl(const llae::write_buffers& buffers) {
		int status = 0;
		for (auto& b:buffers.get_buffers()) {
			status = mbedtls_md_hmac_update(&m_ctx, 
				reinterpret_cast<const unsigned char*>(b.get_base()), b.get_len() );
			if (status != 0)
				break;
		}
		return make_result(status);
	}

	llae::result<void> hmac::sync_update(const llae::write_buffers& buffers) {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("update")) {
			return std::move(e);
		}
		return update_impl(buffers);
	}
	llae::result_promise_ptr<void> hmac::lasync_update(lua::state& l) {
		llae::write_buffers buffers;
		if (!buffers.putm(l, 2)) {
			buffers.reset(l);
			return llae::make_result_promise_string_error<void>("invalid data");
		}
		using work_t = llae::sequental_method_write_buffers_work<void,hmac>;
		return work_t::start(llae::loop::get(l), std::move(buffers), this,&hmac::m_seq,"update", static_cast<llae::result<void>(hmac::*)(const llae::write_buffers&)>(&hmac::update_impl));
	}

	llae::result<llae::buffer_base_ptr> hmac::finish_impl() {
		size_t size = mbedtls_md_get_size(m_info);
		auto digest = llae::buffer::alloc(size);
		auto status = mbedtls_md_hmac_finish(&m_ctx,static_cast<unsigned char*>(digest->get_base()));
		if (status != 0) {
			return status_error::create(status);
		}
		llae::buffer_base_ptr r = std::move(digest);
		return llae::result<llae::buffer_base_ptr>(std::move(r));
	}

	llae::result<llae::buffer_base_ptr> hmac::sync_finish() {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("finish")) {
			return llae::result<llae::buffer_base_ptr>(std::move(e));
		}
		return finish_impl();
	}
	llae::result_promise_ptr<llae::buffer_base_ptr> hmac::async_finish(llae::loop& a) {
		using work_t = llae::sequental_method_work<llae::buffer_base_ptr,hmac>;
		return work_t::start(a,this,&hmac::m_seq,"finish",[](hmac& self){
			return self.finish_impl();
		});
	}


	llae::result<void> hmac::reset() {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("finish")) {
			return std::move(e);
		}
		auto status = mbedtls_md_hmac_reset(&m_ctx);
		return make_result(status);
	}

	
	llae::result<hmac_ptr> hmac::lnew(lua::state& l) {
		auto info = md::get_info(l,1);
		if (!info) {
			return llae::string_error::create("unknown hmac algorithm");
		}
		return common::make_intrusive<hmac>(info);
	}
}
