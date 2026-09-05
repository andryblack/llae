#include "pk.h"
#include "crypto.h"
#include "llae/buffer.h"
#include "llae/work.h"
#include "rsa.h"
#include "random.h"
#include "llae/loop.h"

META_OBJECT_INFO(crypto::pk,meta::object)


namespace crypto {


	class pk_rsa : public rsa_base {
		pk_ptr m_pk;
	public:
		explicit pk_rsa(pk_ptr&& pk,mbedtls_rsa_context* ctx) : m_pk(std::move(pk)) {
			m_ctx = ctx;
		}
	};

	pk::pk() {
		mbedtls_pk_init(&m_ctx);
	}

	pk::~pk() {
		mbedtls_pk_free(&m_ctx);
	}


	llae::result<> pk::parse_public_key(const llae::buffer_view& data) {
		if (data.empty()) {
			return llae::string_error::create("not empty data expected");
		}
		auto res = mbedtls_pk_parse_public_key(&m_ctx,
			static_cast<const unsigned char*>(data.get_base()),data.get_len());
		return make_result(res);
	}

	llae::result<> pk::parse_private_key(const llae::buffer_view& data, std::optional<llae::buffer_view> password) {
		if (data.empty()) {
			return llae::string_error::create("not empty data expected");
		}
		const unsigned char* pwd = nullptr;
		size_t pwd_len = 0;
		if (password.has_value()) {
			pwd = reinterpret_cast<const unsigned char*>(password.value().get_base());
			pwd_len = password.value().get_len();
		}
		auto res = mbedtls_pk_parse_key(&m_ctx,
			static_cast<const unsigned char*>(data.get_base()),data.get_len(),
			pwd,pwd_len,
			&random::read_func, static_cast<void*>(nullptr));
		return make_result(res);
	}

	std::string pk::get_name() {
		auto name = mbedtls_pk_get_name(&m_ctx);
		return name ? std::string(name) : std::string();
	}

	llae::result<rsa_base_ptr> pk::get_rsa() {
		auto rsa = mbedtls_pk_rsa(m_ctx);
		if (rsa) {
			rsa_base_ptr res = common::make_intrusive<pk_rsa>(pk_ptr(this),rsa);
			return res;
		}
		return llae::string_error::create("not rsa");
	}

	llae::result<llae::buffer_base_ptr> pk::sync_encrypt(const llae::buffer_base_ptr& buffer,const random_ptr& random) {
		if (!buffer) {
			return llae::string_error::create("need data");
		}
		if (!random) {
			return llae::string_error::create("need random");
		}
		auto result = llae::buffer::alloc(MBEDTLS_MPI_MAX_SIZE);
		size_t osize = 0;
		auto status = mbedtls_pk_encrypt(&m_ctx,
			static_cast<const unsigned char*>(buffer->get_base()),
			buffer->get_len(),
			static_cast<unsigned char*>(result->get_base()), &osize, result->get_len(),
			&random::read_func, random.get());
		if (status != 0) {
			return status_error::create(status);
		}
		result->set_len(osize);
		llae::buffer_base_ptr r = std::move(result);
		return llae::result<llae::buffer_base_ptr>(std::move(r));
	}
	llae::result<llae::buffer_base_ptr> pk::sync_decrypt(const llae::buffer_base_ptr& buffer,const random_ptr& random) {
		if (!buffer) {
			return llae::string_error::create("need data");
		}
		if (!random) {
			return llae::string_error::create("need random");
		}
		auto result = llae::buffer::alloc(MBEDTLS_MPI_MAX_SIZE);
		size_t osize = 0;
		auto status = mbedtls_pk_decrypt(&m_ctx,
			static_cast<const unsigned char*>(buffer->get_base()),
			buffer->get_len(),
			static_cast<unsigned char*>(result->get_base()), &osize, result->get_len(),
			&random::read_func, random.get());
		if (status != 0) {
			return status_error::create(status);
		}
		result->set_len(osize);
		llae::buffer_base_ptr r = std::move(result);
		return llae::result<llae::buffer_base_ptr>(std::move(r));
	}
	llae::result_promise_ptr<llae::buffer_base_ptr> pk::async_decrypt(llae::loop& a,llae::buffer_base_ptr buffer,random_ptr random) {
		if (!random) {
			random = random_ptr(new crypto::random(entropy_ptr{}));
			random->seed(llae::buffer_view{});
		}
		using work_t = llae::function_work<llae::buffer_base_ptr>;
		return work_t::start(a, [self = pk_ptr(this),lbuffer = std::move(buffer),lrandom = std::move(random)](){
			return self->sync_decrypt(lbuffer, lrandom);
		});
	}
	llae::result_promise_ptr<llae::buffer_base_ptr> pk::async_encrypt(llae::loop& a,llae::buffer_base_ptr buffer,random_ptr random) {
		if (!random) {
			random = random_ptr(new crypto::random(entropy_ptr{}));
			random->seed(llae::buffer_view{});
		}
		using work_t = llae::function_work<llae::buffer_base_ptr>;
		return work_t::start(a, [self = pk_ptr(this),lbuffer = std::move(buffer),lrandom = std::move(random)](){
			return self->sync_encrypt(lbuffer, lrandom);
		});
	}

}
