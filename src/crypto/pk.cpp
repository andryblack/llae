#include "pk.h"
#include "crypto.h"
#include "llae/buffer.h"
#include "llae/work.h"
#include "rsa.h"
#include "random.h"
#include "lua/bind.h"
#include "llae/app.h"
#include "llae/async_bind.h"

META_OBJECT_INFO(crypto::pk,meta::object)


namespace crypto {

	using pk_ptr = common::intrusive_ptr<pk>;

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


	lua::multiret pk::parse_public_key(lua::state& l) {
		auto data = llae::buffer_view::get(l,2,true);
		if (data.empty()) {
			l.argerror(2,"data expected");
			return {0};
		}
		auto res = mbedtls_pk_parse_public_key(&m_ctx,
			static_cast<const unsigned char*>(data.get_base()),data.get_len());
		return mbedtls_result(l,res);
	}

	lua::multiret pk::get_name(lua::state& l) {
		auto name = mbedtls_pk_get_name(&m_ctx);
		l.pushstring(name);
		return {1};
	}

	lua::multiret pk::get_rsa(lua::state& l) {
		auto rsa = mbedtls_pk_rsa(m_ctx);
		if (rsa) {
			lua::push(l,common::intrusive_ptr<pk_rsa>(new pk_rsa(pk_ptr(this),rsa)));
			return {1};
		}
		l.pushnil();
		l.pushstring("not rsa");
		return {2};
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
	llae::result_promise_ptr<llae::buffer_base_ptr> pk::async_encrypt(llae::app& a,llae::buffer_base_ptr buffer,random_ptr random) {
		if (!random) {
			random = random_ptr(new crypto::random(entropy_ptr{}));
			random->seed(llae::buffer_view{});
		}
		using work_t = llae::function_work<llae::buffer_base_ptr>;
		return work_t::start(a, [self = pk_ptr(this),lbuffer = std::move(buffer),lrandom = std::move(random)](){
			return self->sync_encrypt(lbuffer, lrandom);
		});
	}

	void pk::lbind(lua::state& l) {
		lua::bind::function(l,"new",&pk::lnew);
		lua::bind::function(l,"parse_public_key",&pk::parse_public_key);
		lua::bind::function(l,"get_name",&pk::get_name);
		lua::bind::function(l,"get_rsa",&pk::get_rsa);
		llae::async_function(l,"encrypt", &pk::async_encrypt);
	}

	lua::multiret pk::lnew(lua::state& l) {
		lua::push(l,pk_ptr(new pk()));
		return {1};
	}
}
