#include "cipher.h"
#include "llae/buffer.h"
#include "llae/app.h"
#include "uv/work.h"
#include "uv/luv.h"
#include "lua/bind.h"
#include "crypto.h"
#include "llae/write_buffers.h"

META_OBJECT_INFO(crypto::cipher,meta::object)

namespace crypto {

	class cipher::async : public uv::work {
	protected:
		cipher_ptr m_cipher;
		llae::buffer_ptr m_data;
		int m_status = 0;
	public:
		explicit async(cipher_ptr&& m) : m_cipher(std::move(m)) {}
		virtual void on_after_work(int status) override {
            if (llae::app::closed(get_loop())) {
                m_cipher->release();
            } else {
                uv::loop loop(get_loop());
                lua::state& l(llae::app::get(loop).lua());
                m_cipher->on_completed(l,status,m_status,std::move(m_data));
            }
			m_cipher.reset();
		}
	};

	class cipher::buffers_async : public cipher::async {
	protected:
		llae::write_buffers m_buffers;
	public:
		explicit buffers_async(cipher_ptr&& m, llae::write_buffers&& buffers) : cipher::async (std::move(m)),m_buffers(std::move(buffers)) {}
		void reset(lua::state& l) {
            m_buffers.reset(l);
		}
		virtual void on_after_work(int status) override {
			if (llae::app::closed(get_loop())) {
                m_cipher->release();
				m_buffers.release();
            } else {
                uv::loop loop(get_loop());
                lua::state& l(llae::app::get(loop).lua());
                m_cipher->on_completed(l,status,m_status,std::move(m_data));
				m_buffers.reset(l);
            }
			m_cipher.reset();

		}
	};

	class cipher::update_async : public cipher::buffers_async {
	private:
	protected:
	public:
		explicit update_async(cipher_ptr&& m,llae::write_buffers&& buffers) : cipher::buffers_async(std::move(m),std::move(buffers)) {}
		virtual void on_work() override {
			size_t blocksize = mbedtls_cipher_get_block_size(&m_cipher->m_ctx);
			llae::buffer_ptr enc_buffer;
			for (auto& b:m_buffers.get_buffers()) {
				size_t osize = b.get_len() + blocksize;
				if (!enc_buffer || enc_buffer->get_capacity() < osize) {
					enc_buffer = llae::buffer::alloc(osize);
				}
				m_status = mbedtls_cipher_update( &m_cipher->m_ctx,
					reinterpret_cast<const unsigned char*>(b.get_base()), b.get_len(),
					static_cast<unsigned char*>(enc_buffer->get_base()),&osize );
				if (m_status != 0)
					break;
				enc_buffer->set_len(osize);
				if (!m_data) {
					m_data = std::move(enc_buffer);
				} else {
					if (m_data->get_capacity() < (m_data->get_len() + enc_buffer->get_len())) {
						m_data = m_data->realloc(m_data->get_len() + enc_buffer->get_len());
					}
					::memcpy(static_cast<unsigned char*>(m_data->get_base())+m_data->get_len(),enc_buffer->get_base(),enc_buffer->get_len());
					m_data->set_len(m_data->get_len() + enc_buffer->get_len());
				}
			}
		}
	};

	
	class cipher::update_ad_async : public cipher::async {
	private:
		llae::buffer_base_ptr m_buffer;
	public:
		explicit update_ad_async(cipher_ptr&& m,llae::buffer_base_ptr&& buffer) : cipher::async(std::move(m)),m_buffer(std::move(buffer)) {}
		virtual void on_work() override {
			m_status = mbedtls_cipher_update_ad(&m_cipher->m_ctx,
				reinterpret_cast<const unsigned char*>(m_buffer->get_base()), m_buffer->get_len());
		}
	};


	class cipher::finish_async : public cipher::async {
	public:
		explicit finish_async(cipher_ptr&& m) : cipher::async(std::move(m)) {}
		virtual void on_work() override {
			size_t size = mbedtls_cipher_get_block_size(&m_cipher->m_ctx);
			m_data = llae::buffer::alloc(size);
			size_t osize = 0;
			m_status = mbedtls_cipher_finish(&m_cipher->m_ctx,
				static_cast<unsigned char*>(m_data->get_base()),&osize);
			if (m_status == 0) {
				m_data->set_len(osize);
			}
		}
	};

	class cipher::crypt_async : public cipher::async {
	private:
		llae::buffer_base_ptr m_iv;
		llae::buffer_base_ptr m_buffer;
	public:
		explicit crypt_async(cipher_ptr&& m,llae::buffer_base_ptr&& iv,llae::buffer_base_ptr&& buffer) : cipher::async(std::move(m)),m_iv(std::move(iv)),m_buffer(std::move(buffer)) {}
		virtual void on_work() override {
			size_t len = m_buffer->get_len() + mbedtls_cipher_get_block_size(&m_cipher->m_ctx);
			m_data = llae::buffer::alloc(len);
			m_status = mbedtls_cipher_crypt(&m_cipher->m_ctx,
				m_iv ? reinterpret_cast<const unsigned char*>(m_iv->get_base()) : nullptr,
				m_iv ? m_iv->get_len() : 0,
				reinterpret_cast<const unsigned char*>(m_buffer->get_base()), m_buffer->get_len(),
				static_cast<unsigned char*>(m_data->get_base()),&len);
			if (m_status == 0) {
				m_data->set_len(len);
			}
		}
	};

	class cipher::auth_encrypt_async : public cipher::async {
	private:
		llae::buffer_base_ptr m_iv;
		llae::buffer_base_ptr m_ad;
		llae::buffer_base_ptr m_buffer;
		size_t m_tag_len;
	public:
		explicit auth_encrypt_async(cipher_ptr&& m,llae::buffer_base_ptr&& iv,llae::buffer_base_ptr&& ad,llae::buffer_base_ptr&& buffer,size_t tag_len) : cipher::async(std::move(m)),m_iv(std::move(iv)),m_ad(std::move(ad)),m_buffer(std::move(buffer)),m_tag_len(tag_len) {}
		virtual void on_work() override {
			size_t len = m_buffer->get_len() + mbedtls_cipher_get_block_size(&m_cipher->m_ctx) + m_tag_len;
			m_data = llae::buffer::alloc(len);
			m_status = mbedtls_cipher_auth_encrypt_ext(&m_cipher->m_ctx,
				m_iv ? reinterpret_cast<const unsigned char*>(m_iv->get_base()) : nullptr,
				m_iv ? m_iv->get_len() : 0,
				m_ad ? reinterpret_cast<const unsigned char*>(m_ad->get_base()) : nullptr,
				m_ad ? m_ad->get_len() : 0,
				static_cast<const unsigned char*>(m_buffer->get_base()), m_buffer->get_len(),
				static_cast<unsigned char*>(m_data->get_base()), len,
				&len, m_tag_len);
			if (m_status == 0) {
				m_data->set_len(len);
			}
		}
	};

	class cipher::auth_decrypt_async : public cipher::async {
	private:
		llae::buffer_base_ptr m_iv;
		llae::buffer_base_ptr m_ad;
		llae::buffer_base_ptr m_buffer;
		size_t m_tag_len;
	public:
		explicit auth_decrypt_async(cipher_ptr&& m,llae::buffer_base_ptr&& iv,llae::buffer_base_ptr&& ad,llae::buffer_base_ptr&& buffer,size_t tag_len) : cipher::async(std::move(m)),m_iv(std::move(iv)),m_ad(std::move(ad)),m_buffer(std::move(buffer)),m_tag_len(tag_len) {}
		virtual void on_work() override {
			size_t len = m_buffer->get_len() + mbedtls_cipher_get_block_size(&m_cipher->m_ctx) + m_tag_len;
			m_data = llae::buffer::alloc(len);
			m_status = mbedtls_cipher_auth_decrypt_ext(&m_cipher->m_ctx,
				m_iv ? reinterpret_cast<const unsigned char*>(m_iv->get_base()) : nullptr,
				m_iv ? m_iv->get_len() : 0,
				m_ad ? reinterpret_cast<const unsigned char*>(m_ad->get_base()) : nullptr,
				m_ad ? m_ad->get_len() : 0,
				static_cast<const unsigned char*>(m_buffer->get_base()), m_buffer->get_len(),
				static_cast<unsigned char*>(m_data->get_base()), len,
				&len, m_tag_len);
			if (m_status == 0) {
				m_data->set_len(len);
			}
		}
	};

	

	cipher::cipher(const mbedtls_cipher_info_t* info)  {
		mbedtls_cipher_init(&m_ctx);
		mbedtls_cipher_setup(&m_ctx,info);
	}

	cipher::~cipher() {
		mbedtls_cipher_free(&m_ctx);
	}

	lua::multiret cipher::set_iv(lua::state& l) {
		auto iv = llae::buffer::get(l,2);
		if (!iv) {
			l.argerror(2,"need iv data");
			return {0};
		}
		auto ret = mbedtls_cipher_set_iv(&m_ctx,
			static_cast<const unsigned char*>(iv->get_base()),iv->get_len());
		if (ret != 0) {
            l.pushnil();
	        push_error(l,"set_iv failed, code:%d, %s",ret);
	        return {2};
        }
        l.pushboolean(true);
        return {1};
	}
	lua::multiret cipher::set_key(lua::state& l) {
		auto key = llae::buffer::get(l,2);
		mbedtls_operation_t op = mbedtls_operation_t(l.optinteger(3,MBEDTLS_DECRYPT));
		if (!key) {
			l.argerror(2,"need key data");
			return {0};
		}
		auto ret = mbedtls_cipher_setkey(&m_ctx,
			static_cast<const unsigned char*>(key->get_base()),
                                         static_cast<int>(key->get_len()*8),op);
		if (ret != 0) {
            l.pushnil();
	        push_error(l,"set_key failed, code:%d, %s",ret);
	        return {2};
        }
        l.pushboolean(true);
        return {1};
	}
	lua::multiret cipher::set_padding(lua::state& l) {
		mbedtls_cipher_padding_t padding = static_cast<mbedtls_cipher_padding_t>(l.checkinteger(2));
		auto ret = mbedtls_cipher_set_padding_mode(&m_ctx,padding);
		if (ret != 0) {
            l.pushnil();
	        push_error(l,"set_padding failed, code:%d, %s",ret);
	        return {2};
        }
        l.pushboolean(true);
        return {1};
	}

	lua::multiret cipher::reset(lua::state& l) {
		auto ret = mbedtls_cipher_reset(&m_ctx);
		if (ret != 0) {
            l.pushnil();
	        push_error(l,"reset failed, code:%d, %s",ret);
	        return {2};
        }
        l.pushboolean(true);
        return {1};
	}

	lua::multiret cipher::update(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("cipher::update is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("cipher::update operation in progress");
			return {2};
		}
		{
            llae::write_buffers buffers;
            l.pushvalue(2);
            if (!buffers.put(l)) {
                buffers.reset(l);
                l.pushnil();
                l.pushstring("cipher::update invalid data");
                return {2};
            }
            if (buffers.empty()) {
                l.pushlstring("", 0);
                return {1};
            }

			common::intrusive_ptr<update_async> req{new update_async(cipher_ptr(this),std::move(buffers))};
						
			l.pushthread();
			m_cont.set(l);
			
			int r = req->queue_work(llae::app::get(l).loop());
			if (r < 0) {
                req->reset(l);
				m_cont.reset(l);
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}

	lua::multiret cipher::update_ad(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("cipher::update_ad is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("cipher::update_ad operation in progress");
			return {2};
		}
		{
            auto buffer = llae::buffer_base::get(l,2);
			if (!buffer) {
				l.pushnil();
				l.pushstring("cipher::update_ad invalid data");
				return {2};
			}

			common::intrusive_ptr<update_ad_async> req{new update_ad_async(cipher_ptr(this),std::move(buffer))};
						
			l.pushthread();
			m_cont.set(l);
			
			int r = req->queue_work(llae::app::get(l).loop());
			if (r < 0) {
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}

	lua::multiret cipher::finish(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("cipher::finish is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("cipher::finish operation in progress");
			return {2};
		}
		{
			l.pushthread();
			m_cont.set(l);
			common::intrusive_ptr<finish_async> req{new finish_async(cipher_ptr(this))};
			
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

	lua::multiret cipher::crypt(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("cipher::crypt is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("cipher::crypt operation in progress");
			return {2};
		}
		{
			auto iv = llae::buffer_base::get(l,2);
            auto buffer = llae::buffer_base::get(l,3);
			if (!buffer) {
				l.pushnil();
				l.pushstring("cipher::crypt invalid data");
				return {2};
			}

			l.pushthread();
			m_cont.set(l);
			common::intrusive_ptr<crypt_async> req{new crypt_async(cipher_ptr(this),std::move(iv),std::move(buffer))};
			
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

	lua::multiret cipher::auth_encrypt(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("cipher::auth_encrypt is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("cipher::auth_encrypt operation in progress");
			return {2};
		}
		{
			auto iv = llae::buffer_base::get(l,2);
			auto ad = llae::buffer_base::get(l,3);
            auto buffer = llae::buffer_base::get(l,4);
			if (!buffer) {
				l.pushnil();
				l.pushstring("cipher::auth_encrypt invalid data");
				return {2};
			}
			auto tag_len = l.optinteger(5,0);

			l.pushthread();
			m_cont.set(l);
			common::intrusive_ptr<auth_encrypt_async> req{new auth_encrypt_async(cipher_ptr(this),std::move(iv),std::move(ad),std::move(buffer),tag_len)};
			
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

	lua::multiret cipher::auth_decrypt(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("cipher::auth_decrypt is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("cipher::auth_decrypt operation in progress");
			return {2};
		}
		{
			auto iv = llae::buffer_base::get(l,2);
			auto ad = llae::buffer_base::get(l,3);
            auto buffer = llae::buffer_base::get(l,4);
			if (!buffer) {
				l.pushnil();
				l.pushstring("cipher::auth_decrypt invalid data");
				return {2};
			}
			auto tag_len = l.optinteger(5,0);

			l.pushthread();
			m_cont.set(l);
			common::intrusive_ptr<auth_decrypt_async> req{new auth_decrypt_async(cipher_ptr(this),std::move(iv),std::move(ad),std::move(buffer),tag_len)};
			
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


	void cipher::on_completed(lua::state& l,int uvstatus,int mbedlsstatus,llae::buffer_ptr&& data) {
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
            push_error(toth,"operation failed, code:%d, %s",mbedlsstatus);
            nres = 2;
        } else {
			if (data) {
				lua::push(toth,std::move(data));
			} else {
				toth.pushboolean(true);
			}
            nres = 1;
        }
		auto s = toth.resume(l,nres);
		if (s != lua::status::ok && s != lua::status::yield) {
			llae::app::show_error(toth,s);
		}
		l.pop(1);// thread
	}

	int cipher::get_block_size() const {
		return mbedtls_cipher_get_block_size(&m_ctx);
	}
	int cipher::get_iv_size() const {
		return mbedtls_cipher_get_iv_size(&m_ctx);
	}
	lua::multiret cipher::write_tag(lua::state& l) {
		auto tag_len = l.checkinteger(2);
		auto data = llae::buffer::alloc(tag_len);
		auto ret = mbedtls_cipher_write_tag(&m_ctx,
			reinterpret_cast<unsigned char*>(data->get_base()), tag_len);
		if (ret != 0) {
			l.pushnil();
			push_error(l,"write_tag failed, code:%d, %s",ret);
			return {2};
		}
		lua::push(l,std::move(data));
		return {1};
	}
	lua::multiret cipher::check_tag(lua::state& l) {
		auto tag = llae::buffer_view::get(l,2, true);
		auto ret = mbedtls_cipher_check_tag(&m_ctx,
			reinterpret_cast<const unsigned char*>(tag.get_base()), tag.get_len());
		if (ret != 0) {
			l.pushnil();
			push_error(l,"write_tag failed, code:%d, %s",ret);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	void cipher::lbind(lua::state& l) {
		lua::bind::function(l,"new",&cipher::lnew);
		lua::bind::function(l,"get_block_size",&cipher::get_block_size);
		lua::bind::function(l,"get_iv_size",&cipher::get_iv_size);
		lua::bind::function(l,"set_iv",&cipher::set_iv);
		lua::bind::function(l,"set_key",&cipher::set_key);
		lua::bind::function(l,"set_padding",&cipher::set_padding);
		lua::bind::function(l,"reset",&cipher::reset);
		lua::bind::function(l,"update",&cipher::update);
		lua::bind::function(l,"update_ad",&cipher::update_ad);
		lua::bind::function(l,"finish",&cipher::finish);
		lua::bind::function(l,"write_tag",&cipher::write_tag);
		lua::bind::function(l,"check_tag",&cipher::check_tag);
		lua::bind::function(l,"crypt",&cipher::crypt);
		lua::bind::function(l,"auth_encrypt",&cipher::auth_encrypt);
		lua::bind::function(l,"auth_decrypt",&cipher::auth_decrypt);
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
