#include "cipher.h"
#include "common/intrusive_ptr.h"
#include "llae/buffer.h"
#include "llae/app.h"
#include "llae/promise.h"
#include "llae/result.h"
#include "llae/sequental.h"
#include "llae/work.h"
#include "crypto.h"
#include "llae/write_buffers.h"
#include "llae/async_bind.h"

META_OBJECT_INFO(crypto::cipher,meta::object)

namespace crypto {


	cipher::cipher(const mbedtls_cipher_info_t* info)  {
		mbedtls_cipher_init(&m_ctx);
		mbedtls_cipher_setup(&m_ctx,info);
	}

	cipher::~cipher() {
		mbedtls_cipher_free(&m_ctx);
	}

	llae::result<void> cipher::set_iv(const llae::buffer_view& iv) {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("set_iv")) {
			return llae::result<void>(std::move(e));
		}
		auto ret = mbedtls_cipher_set_iv(&m_ctx,
			static_cast<const unsigned char*>(iv.get_base()),iv.get_len());
		return make_result(ret);
	}
	llae::result<void> cipher::set_key(const llae::buffer_view& key,mbedtls_operation_t op) {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("set_key")) {
			return llae::result<void>(std::move(e));
		}
		auto ret = mbedtls_cipher_setkey(&m_ctx,
			static_cast<const unsigned char*>(key.get_base()),
                                         static_cast<int>(key.get_len()*8),op);
		return make_result(ret);
	}
	llae::result<void> cipher::set_padding(mbedtls_cipher_padding_t padding) {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("set_padding")) {
			return llae::result<void>(std::move(e));
		}
		auto ret = mbedtls_cipher_set_padding_mode(&m_ctx,padding);
		return make_result(ret);
	}

	llae::result<void> cipher::reset(lua::state& l) {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("reset")) {
			return llae::result<void>(std::move(e));
		}
		auto ret = mbedtls_cipher_reset(&m_ctx);
		return make_result(ret);
	}

	llae::result<llae::buffer_base_ptr> cipher::update_impl(const llae::write_buffers& buffers) {
		size_t blocksize = mbedtls_cipher_get_block_size(&m_ctx);
		llae::buffer_ptr data;
		llae::buffer_ptr enc_buffer;
		int status = 0;
		for (auto& b:buffers.get_buffers()) {
			size_t osize = b.get_len() + blocksize;
			if (!enc_buffer || enc_buffer->get_capacity() < osize) {
				enc_buffer = llae::buffer::alloc(osize);
			}
			status = mbedtls_cipher_update( &m_ctx,
				reinterpret_cast<const unsigned char*>(b.get_base()), b.get_len(),
				static_cast<unsigned char*>(enc_buffer->get_base()),&osize );
			if (status != 0)
				break;
			enc_buffer->set_len(osize);
			if (!data) {
				data = std::move(enc_buffer);
			} else {
				if (data->get_capacity() < (data->get_len() + enc_buffer->get_len())) {
					data = data->realloc(data->get_len() + enc_buffer->get_len());
				}
				::memcpy(static_cast<unsigned char*>(data->get_base())+data->get_len(),enc_buffer->get_base(),enc_buffer->get_len());
				data->set_len(data->get_len() + enc_buffer->get_len());
			}
		}
		if (status != 0) {
			return status_error::create(status);
		}
		llae::buffer_base_ptr r = std::move(data);
		return llae::result<llae::buffer_base_ptr>(std::move(r));
	}
	llae::result<llae::buffer_base_ptr> cipher::sync_update(const llae::write_buffers& buffers) {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("update")) {
			return llae::result<llae::buffer_base_ptr>(std::move(e));
		}
		return update_impl(buffers);
	}

	llae::result_promise_ptr<llae::buffer_base_ptr> cipher::lasync_update(lua::state& l) {
		
		llae::write_buffers buffers;
		if (!buffers.putm(l, 2)) {
			return llae::make_result_promise_string_error<llae::buffer_base_ptr>("invalid data");
		}
		using work_t = llae::sequental_method_write_buffers_work<llae::buffer_base_ptr,cipher>;
		return work_t::start(llae::app::get(l), std::move(buffers), this,&cipher::m_seq,"update", static_cast<llae::result<llae::buffer_base_ptr>(cipher::*)(const llae::write_buffers&)>(&cipher::update_impl));
	}

	llae::result<void> cipher::update_ad_impl(const llae::buffer_base_ptr& data) {
		auto status = mbedtls_cipher_update_ad(&m_ctx,
			reinterpret_cast<const unsigned char*>(data->get_base()), data->get_len());
		return make_result(status);
	}
	llae::result<void> cipher::sync_update_ad(const llae::buffer_base_ptr& data) {
		if (!data) {
			return llae::string_error::create("need data");
		}
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("update_ad")) {
			return llae::result<void>(std::move(e));
		}
		return update_ad_impl(data);
	}

	llae::result_promise_ptr<void> cipher::async_update_ad(llae::app& a,llae::buffer_base_ptr&& data) {
		if (!data) {
			return llae::make_result_promise_string_error<void>("need data");
		}
		using work_t = llae::sequental_method_work<void,cipher>;
		return work_t::start(a, this, &cipher::m_seq, "update_ad", [d = std::move(data)](cipher& self){
			return self.update_ad_impl(d);
		});
	}

	llae::result<llae::buffer_base_ptr> cipher::finish_impl() {
		size_t size = mbedtls_cipher_get_block_size(&m_ctx);
		auto data = llae::buffer::alloc(size);
		size_t osize = 0;
		auto status = mbedtls_cipher_finish(&m_ctx,
			static_cast<unsigned char*>(data->get_base()),&osize);
		if (status == 0) {
			data->set_len(osize);
			llae::buffer_base_ptr r = std::move(data);
			return llae::result<llae::buffer_base_ptr>(std::move(r));
		}
		return status_error::create(status);
	}
	llae::result<llae::buffer_base_ptr> cipher::sync_finish() {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("finish")) {
			return llae::result<llae::buffer_base_ptr>(std::move(e));
		}
		return finish_impl();
	}

	llae::result_promise_ptr<llae::buffer_base_ptr> cipher::async_finish(llae::app& a) {
		using work_t = llae::sequental_method_work<llae::buffer_base_ptr,cipher>;
		return work_t::start(a, this, &cipher::m_seq, "finish", [](cipher& self){
			return self.finish_impl();
		});
	}

	llae::result<llae::buffer_base_ptr> cipher::crypt_impl(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& buffer) {
		size_t len = buffer->get_len() + mbedtls_cipher_get_block_size(&m_ctx);
		auto data = llae::buffer::alloc(len);
		auto status = mbedtls_cipher_crypt(&m_ctx,
			iv ? reinterpret_cast<const unsigned char*>(iv->get_base()) : nullptr,
			iv ? iv->get_len() : 0,
			reinterpret_cast<const unsigned char*>(buffer->get_base()), buffer->get_len(),
			static_cast<unsigned char*>(data->get_base()),&len);
		if (status == 0) {
			data->set_len(len);
			llae::buffer_base_ptr r = std::move(data);
			return llae::result<llae::buffer_base_ptr>(std::move(r));
		}
		return status_error::create(status);
	}
	llae::result<llae::buffer_base_ptr> cipher::sync_crypt(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& buffer) {
		if (!buffer) {
			return llae::string_error::create("need data");
		}
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("crypt")) {
			return std::move(e);
		}
		return crypt_impl(iv, buffer);
	}

	llae::result_promise_ptr<llae::buffer_base_ptr> cipher::async_crypt(llae::app& a, llae::buffer_base_ptr iv, llae::buffer_base_ptr buffer) {
		if (!buffer) {
			return llae::make_result_promise_string_error<llae::buffer_base_ptr> ("need data");
		}
		using work_t = llae::sequental_method_work<llae::buffer_base_ptr,cipher>;
		return work_t::start(a, this, &cipher::m_seq, "crypt", [liv = std::move(iv),lbuffer = std::move(buffer)](cipher& self){
			return self.crypt_impl(liv,lbuffer);
		});
	}

	llae::result<llae::buffer_base_ptr> cipher::auth_encrypt_impl(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len) {
		size_t len = buffer->get_len() + mbedtls_cipher_get_block_size(&m_ctx) + tag_len;
		auto data = llae::buffer::alloc(len);
		auto status = mbedtls_cipher_auth_encrypt_ext(&m_ctx,
			iv ? reinterpret_cast<const unsigned char*>(iv->get_base()) : nullptr,
			iv ? iv->get_len() : 0,
			ad ? reinterpret_cast<const unsigned char*>(ad->get_base()) : nullptr,
			ad ? ad->get_len() : 0,
			static_cast<const unsigned char*>(buffer->get_base()), buffer->get_len(),
			static_cast<unsigned char*>(data->get_base()), len,
			&len, tag_len);
		if (status == 0) {
			data->set_len(len);
			llae::buffer_base_ptr r = std::move(data);
			return llae::result<llae::buffer_base_ptr>(std::move(r));
		}
		return status_error::create(status);
	}
	llae::result<llae::buffer_base_ptr> cipher::sync_auth_encrypt(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len) {
		if (!buffer) {
			return llae::string_error::create("need data");
		}
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("auth_encrypt")) {
			return std::move(e);
		}
		return auth_encrypt_impl(iv, ad, buffer, tag_len);
	}
	llae::result_promise_ptr<llae::buffer_base_ptr> cipher::async_auth_encrypt(llae::app& a, llae::buffer_base_ptr iv, llae::buffer_base_ptr ad, llae::buffer_base_ptr buffer, size_t tag_len) {
		if (!buffer) {
			return llae::make_result_promise_string_error<llae::buffer_base_ptr> ("need data");
		}
		using work_t = llae::sequental_method_work<llae::buffer_base_ptr,cipher,6>;
		return work_t::start(a, this, &cipher::m_seq, "auth_encrypt", [liv = std::move(iv),lad = std::move(ad), lbuffer = std::move(buffer), tag_len](cipher& self){
			return self.auth_encrypt_impl(liv,lad,lbuffer,tag_len);
		});
	}
	llae::result<llae::buffer_base_ptr> cipher::auth_decrypt_impl(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len) {
		size_t len = buffer->get_len() + mbedtls_cipher_get_block_size(&m_ctx) + tag_len;
		auto data = llae::buffer::alloc(len);
		auto status = mbedtls_cipher_auth_decrypt_ext(&m_ctx,
			iv ? reinterpret_cast<const unsigned char*>(iv->get_base()) : nullptr,
			iv ? iv->get_len() : 0,
			ad ? reinterpret_cast<const unsigned char*>(ad->get_base()) : nullptr,
			ad ? ad->get_len() : 0,
			static_cast<const unsigned char*>(buffer->get_base()), buffer->get_len(),
			static_cast<unsigned char*>(data->get_base()), len,
			&len, tag_len);
		if (status == 0) {
			data->set_len(len);
			llae::buffer_base_ptr r = std::move(data);
			return llae::result<llae::buffer_base_ptr>(std::move(r));
		}
		return status_error::create(status);
	}
	llae::result<llae::buffer_base_ptr> cipher::sync_auth_decrypt(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len) {
		if (!buffer) {
			return llae::string_error::create("need data");
		}
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("auth_decrypt")) {
			return std::move(e);
		}
		return auth_decrypt_impl(iv, ad, buffer, tag_len);
	}
	llae::result_promise_ptr<llae::buffer_base_ptr> cipher::async_auth_decrypt(llae::app& a, llae::buffer_base_ptr iv, llae::buffer_base_ptr ad, llae::buffer_base_ptr buffer, size_t tag_len) {
		if (!buffer) {
			return llae::make_result_promise_string_error<llae::buffer_base_ptr> ("need data");
		}
		using work_t = llae::sequental_method_work<llae::buffer_base_ptr,cipher,6>;
		return work_t::start(a, this, &cipher::m_seq, "auth_decrypt", [liv = std::move(iv),lad = std::move(ad), lbuffer = std::move(buffer), tag_len](cipher& self){
			return self.auth_decrypt_impl(liv,lad,lbuffer,tag_len);
		});
	}
		

	int cipher::get_block_size() const {
		return mbedtls_cipher_get_block_size(&m_ctx);
	}
	int cipher::get_iv_size() const {
		return mbedtls_cipher_get_iv_size(&m_ctx);
	}
	llae::result<llae::buffer_base_ptr> cipher::write_tag(size_t tag_len) {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("write_tag")) {
			return std::move(e);
		}
		auto data = llae::buffer::alloc(tag_len);
		auto ret = mbedtls_cipher_write_tag(&m_ctx,
			reinterpret_cast<unsigned char*>(data->get_base()), tag_len);
		if (ret != 0) {
			return status_error::create(ret);
		}
		llae::buffer_base_ptr b = std::move(data);
		return llae::result<llae::buffer_base_ptr>(std::move(b));
	}
	llae::result<void> cipher::check_tag(const llae::buffer_view& data) {
		llae::sequental_scope s{m_seq};
		if (auto e = s.start("write_tag")) {
			return llae::result<void>(std::move(e));
		}
		auto ret = mbedtls_cipher_check_tag(&m_ctx,
			reinterpret_cast<const unsigned char*>(data.get_base()), data.get_len());
		return make_result(ret);
	}

	lua::multiret cipher::lnew(lua::state& l) {
		const char* alg = l.checkstring(1);
		auto info = mbedtls_cipher_info_from_string(alg);
		if (!info) {
			l.pushnil();
			l.pushfstring("unknown cipher algorithm '%s'",alg);
			return {2};
		}
		lua::push(l,cipher_ptr(new cipher(info)));
		return {1};
	}

}
