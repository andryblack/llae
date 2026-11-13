#include "md.h"
#include <string>
#include "common/intrusive_ptr.h"
#include "crypto.h"
#include "llae/error.h"
#include "llae/promise.h"
#include "llae/result.h"
#include "llae/sequental.h"
#include "llae/buffer.h"
#include "llae/loop.h"
#include "llae/write_buffers.h"
#include "llae/work.h"

META_OBJECT_INFO(crypto::md,meta::object)

namespace crypto {


	md::md(const mbedtls_md_info_t* info) : m_info(info) {
		mbedtls_md_init(&m_ctx);
		mbedtls_md_setup(&m_ctx,m_info,0);
	}

	const mbedtls_md_info_t* md::find_info(std::string_view algorithm) {
		std::string name(algorithm);
		return mbedtls_md_info_from_string(name.c_str());
	}

	const mbedtls_md_info_t* md::find_info(mbedtls_md_type_t type) {
		return mbedtls_md_info_from_type(type);
	}

	md_ptr md::create(mbedtls_md_type_t type) {
		auto info = find_info(type);
		if (!info) {
			return md_ptr{};
		}
		return md_ptr(new md(info));
	}

	md_ptr md::create(const char* type) {
		auto info = find_info(type ? std::string_view(type) : std::string_view{});
		if (!info) {
			return md_ptr{};
		}
		return md_ptr(new md(info));
	}

	md::~md() {
		mbedtls_md_free(&m_ctx);
	}
	

	llae::result<size_t> md::get_length(std::string_view algorithm) {
		auto info = find_info(algorithm);
		if (!info) {
			return llae::string_error::create("unknown md algorithm");
		}
		return mbedtls_md_get_size(info);
	}

	llae::result<size_t> md::get_length(mbedtls_md_type_t type) {
		auto info = find_info(type);
		if (!info) {
			return llae::string_error::create("unknown md algorithm");
		}
		return mbedtls_md_get_size(info);
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

	llae::result_promise_ptr<void> md::async_update(llae::loop& a,llae::buffer_base_ptr&& data) {
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

	llae::result_promise_ptr<void> md::async_update_buffers(llae::loop& a,llae::write_buffers&& buffers) {
		auto e = try_start();
		if (e) {
			return llae::result_promise_forward_error<void>(std::move(e));
		}
		using work_t = llae::sequental_method_write_buffers_work<void,md>;
		return work_t::start(a, std::move(buffers), this, &md::m_seq, "update",
			static_cast<llae::result<void>(md::*)(const llae::write_buffers&)>(&md::update_impl));
	}

	llae::result_promise_ptr<llae::buffer_base_ptr> md::async_finish(llae::loop& a) {
		using work_t = llae::sequental_method_work<llae::buffer_base_ptr,md>;
		return work_t::start(a,this,&md::m_seq,"finish",[](md& self){
			return self.finish_impl();
		});
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
}
