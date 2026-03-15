#include "md.h"
#include <memory>
#include "common/intrusive_ptr.h"
#include "crypto.h"
#include "llae/error.h"
#include "llae/promise.h"
#include "llae/result.h"
#include "llae/sequental.h"
#include "lua/state.h"
#include "llae/buffer.h"
#include "llae/app.h"
#include "lua/bind.h"
#include "lua/stack.h"
#include "llae/write_buffers.h"
#include "llae/work.h"
#include "llae/async_bind.h"

META_OBJECT_INFO(crypto::md,meta::object)

namespace crypto {


	md::md(const mbedtls_md_info_t* info) : m_info(info) {
		mbedtls_md_init(&m_ctx);
		mbedtls_md_setup(&m_ctx,m_info,0);
	}

	common::intrusive_ptr<md> md::create(mbedtls_md_type_t type) {
		auto info = mbedtls_md_info_from_type(type);
		if (!info) {
			return {};
		}
		return common::make_intrusive<md>(info);
	}
	
	common::intrusive_ptr<md> md::create(const char* type) {
		auto info = mbedtls_md_info_from_string(type);
		if (!info) {
			return {};
		}
		return common::make_intrusive<md>(info);
	}

	md::~md() {
		mbedtls_md_free(&m_ctx);
	}
	

	void md::lbind(lua::state& l) {
		lua::bind::function(l,"new",&md::lnew);
		lua::bind::function(l,"get_length",&md::get_length);
		llae::async_function(l,"update",&md::lasync_update);
		llae::async_function(l,"finish",&md::async_finish);
	}

	const mbedtls_md_info_t* md::get_info(lua::state& l, int idx) {
		if (l.get_type(idx) == lua::value_type::string) {
			return mbedtls_md_info_from_string(l.tostring(idx));
		}
		if (l.get_type(idx) == lua::value_type::number) {
			return mbedtls_md_info_from_type(static_cast<mbedtls_md_type_t>(l.tointeger(idx)));
		}
		return nullptr;
	}

	lua::multiret md::get_length(lua::state& l) {
		auto info = get_info(l,1);
		if (!info) {
			l.pushnil();
			l.pushfstring("unknown md algorithm");
			return {2};
		}
		l.pushinteger(mbedtls_md_get_size(info));
		return {1};
	}

	llae::result<void> md::update_impl(const llae::buffer_base_ptr& data) {
		if (!data) {
			return llae::string_error::create("need data");
		}
		if (data->get_len() == 0) {
			return llae::result<void>{};
		}
		auto r = mbedtls_md_update(&m_ctx, 
			reinterpret_cast<const unsigned char*>(data->get_base()), data->get_len() );
		return make_result(r);
	}
	llae::result<void> md::sync_update(const llae::buffer_base_ptr& data) {
		if (!data) {
			return llae::string_error::create("need data");
		}
		if (auto e = try_start()) {
			return std::move(e);
		}
		llae::sequental_scope l{m_seq};
		if (auto e = l.start("update")) {
			return std::move(e);
		}
		return update_impl(data);
	}
	llae::result<void> md::update_impl(const llae::write_buffers& data) {
		for (auto& b:data.get_buffers()) {
			if (b.get_len() == 0)
				continue;
			auto status = mbedtls_md_update(&m_ctx, 
				reinterpret_cast<const unsigned char*>(b.get_base()), b.get_len() );
			if (status != 0) {
				return status_error::create(status);
			}
		}
		return llae::result<void>{};
	}
	llae::result<void> md::sync_update(const llae::write_buffers& data) {
		if (auto e = try_start()) {
			return std::move(e);
		}
		llae::sequental_scope l{m_seq};
		if (auto e = l.start("update")) {
			return std::move(e);
		}
		return update_impl(data);
	}
	llae::result<llae::buffer_base_ptr> md::finish_impl() {
		size_t size = mbedtls_md_get_size(m_info);
		auto digest = llae::buffer::alloc(size);
		auto r = mbedtls_md_finish(&m_ctx,static_cast<unsigned char*>(digest->get_base()));
		if (r != 0) {
			return status_error::create(r);
		}
		llae::buffer_base_ptr res(std::move(digest));
		return llae::result<llae::buffer_base_ptr>{std::move(res)};
	}
	llae::result<llae::buffer_base_ptr> md::sync_finish() {
		llae::sequental_scope l{m_seq};
		if (auto e = l.start("finish")) {
			return std::move(e);
		}
		return finish_impl();
	}

	llae::result_promise_ptr<void> md::async_update(llae::app& a,llae::buffer_base_ptr&& data) {
		if (!data) {
			return llae::make_result_promise_string_error<void>("need data");
		}
		auto e = try_start();
		if (e) {
			return llae::result_promise_forward_error<void>(std::move(e));
		}
		using work_t = llae::sequental_method_work<void,md>;
		return work_t::start(a, this, &md::m_seq, "update", [d = std::move(data)](md& self){
			return self.update_impl(d);
		});
	}

	llae::result_promise_ptr<llae::buffer_base_ptr> md::async_finish(llae::app& a) {
		using work_t = llae::sequental_method_work<llae::buffer_base_ptr,md>;
		return work_t::start(a,this,&md::m_seq,"finish",[](md& self){
			return self.finish_impl();
		});
	}

	llae::result_promise_ptr<void> md::lasync_update(lua::state& l) {
		auto e = try_start();
		if (e) {
			return llae::result_promise_forward_error<void>(std::move(e));
		}
		llae::write_buffers buffers;
		if (!buffers.putm(l, 2)) {
			buffers.reset(l);
			return llae::make_result_promise_string_error<void>("invalid data");
		}
		using work_t = llae::sequental_method_write_buffers_work<void,md>;
		return work_t::start(llae::app::get(l), std::move(buffers), this,&md::m_seq,"update", static_cast<llae::result<void>(md::*)(const llae::write_buffers&)>(&md::update_impl));
	}

	llae::error_ptr md::try_start() {
		if (!m_stated) {
			llae::sequental_scope l{m_seq};
			if (auto e = l.start("try_start")) {
				return std::move(e);
			}
			int r = mbedtls_md_starts(&m_ctx);
			if (r != 0) {
				return common::make_intrusive<status_error>(r);
			}
			m_stated = true;
		}
		return {};
	}

	lua::multiret md::lnew(lua::state& l) {
		auto info = get_info(l,1);
		if (!info) {
			l.pushnil();
			l.pushfstring("unknown md algorithm");
			return {2};
		}
		lua::push(l,common::intrusive_ptr<md>(new md(info)));
		return {1};
	}
}
