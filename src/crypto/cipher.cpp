#include "cipher.h"
#include "uv/buffer.h"
#include "llae/app.h"
#include "uv/work.h"
#include "uv/luv.h"
#include "lua/bind.h"
#include "crypto.h"

META_OBJECT_INFO(crypto::cipher,meta::object)

namespace crypto {

	class cipher::async : public uv::work {
	protected:
		cipher_ptr m_cipher;
		uv::buffer_ptr m_data;
		int m_status = 0;
	public:
		explicit async(cipher_ptr&& m) : m_cipher(std::move(m)) {}
	};

	class cipher::update_async : public cipher::async {
	private:
		uv::write_buffers m_buffers;
	public:
		explicit update_async(cipher_ptr&& m) : cipher::async(std::move(m)) {}
		virtual void on_work() override {
			size_t blocksize = mbedtls_cipher_get_block_size(&m_cipher->m_ctx);
			uv::buffer_ptr enc_buffer;
			for (auto& b:m_buffers.get_buffers()) {
				size_t osize = b.len + blocksize;
				if (!enc_buffer || enc_buffer->get_capacity() < osize) {
					enc_buffer = uv::buffer::alloc(osize);
				}
				m_status = mbedtls_cipher_update(&m_cipher->m_ctx, 
					reinterpret_cast<const unsigned char*>(b.base), b.len,
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
		bool put(lua::state& l) {
			return m_buffers.put(l);
		}
		virtual void on_after_work(int status) override {
            if (llae::app::closed(get_loop())) {
                m_buffers.release();
                m_cipher->release();
            } else {
                uv::loop loop(get_loop());
                lua::state& l(llae::app::get(loop).lua());
                m_buffers.reset(l);
                m_cipher->on_completed(l,status,m_status,std::move(m_data));
            }
		}
	};


	class cipher::finish_async : public cipher::async {
	public:
		explicit finish_async(cipher_ptr&& m) : cipher::async(std::move(m)) {}
		virtual void on_work() override {
			size_t size = mbedtls_cipher_get_block_size(&m_cipher->m_ctx);
			m_data = uv::buffer::alloc(size);
			size_t osize = 0;
			m_status = mbedtls_cipher_finish(&m_cipher->m_ctx,
				static_cast<unsigned char*>(m_data->get_base()),&osize);
			if (m_status == 0) {
				m_data->set_len(osize);
			}
		}
		virtual void on_after_work(int status) override {
            if (llae::app::closed(get_loop())) {
                m_cipher->release();
            } else {
                uv::loop loop(get_loop());
                lua::state& l(llae::app::get(loop).lua());
                m_cipher->on_completed(l,status,m_status,std::move(m_data));
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
		auto iv = uv::buffer::get(l,2);
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
		auto key = uv::buffer::get(l,2);
		mbedtls_operation_t op = mbedtls_operation_t(l.optinteger(3,MBEDTLS_DECRYPT));
		if (!key) {
			l.argerror(2,"need key data");
			return {0};
		}
		auto ret = mbedtls_cipher_setkey(&m_ctx,
			static_cast<const unsigned char*>(key->get_base()),key->get_len()*8,op);
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
			common::intrusive_ptr<update_async> req{new update_async(cipher_ptr(this))};
			l.pushvalue(2);
			if (!req->put(l)) {
				l.pushnil();
				l.pushstring("cipher::update invalid data");
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

	void cipher::on_completed(lua::state& l,int uvstatus,int mbedlsstatus,uv::buffer_ptr&& data) {
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
            lua::push(toth,std::move(data));
            nres = 1;
        }
		auto s = toth.resume(l,nres);
		if (s != lua::status::ok && s != lua::status::yield) {
			llae::app::show_error(toth,s);
		}
		l.pop(1);// thread
	}

	void cipher::lbind(lua::state& l) {
		lua::bind::function(l,"new",&cipher::lnew);
		lua::bind::function(l,"set_iv",&cipher::set_iv);
		lua::bind::function(l,"set_key",&cipher::set_key);
		lua::bind::function(l,"set_padding",&cipher::set_padding);
		lua::bind::function(l,"reset",&cipher::reset);
		lua::bind::function(l,"update",&cipher::update);
		lua::bind::function(l,"finish",&cipher::finish);
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