#include "pk.h"
#include "crypto.h"
#include "rsa.h"
#include "random.h"
#include "lua/bind.h"
#include "llae/app.h"
#include "uv/work.h"
#include "uv/luv.h"

META_OBJECT_INFO(crypto::pk,meta::object)


namespace crypto {

	using pk_ptr = common::intrusive_ptr<pk>;


	class pk::async : public uv::work {
	protected:
		pk_ptr m_pk;
		llae::buffer_base_ptr m_src;
		llae::buffer_ptr m_result;
		random_ptr m_random;
		int m_status = 0;
	public:
		explicit async(pk_ptr&& m,llae::buffer_base_ptr&& src,random_ptr&& r) : m_pk(std::move(m)), m_src(std::move(src)),m_random(std::move(r)) {}
		virtual void on_after_work(int status) override {
            if (llae::app::closed(get_loop())) {
                m_pk->release();
            } else {
                uv::loop loop(get_loop());
                lua::state& l(llae::app::get(loop).lua());
                m_pk->on_completed(l,status,m_status,std::move(m_result));
            }
		}
	};

	class pk::encrypt_async : public pk::async {
	public:
		explicit encrypt_async(pk_ptr&& m,llae::buffer_base_ptr&& src,random_ptr&& r) : pk::async(std::move(m),std::move(src),std::move(r)) {}
		virtual void on_work() override {
			m_result = llae::buffer::alloc(MBEDTLS_MPI_MAX_SIZE);
			size_t osize = 0;
			m_status = mbedtls_pk_encrypt(&m_pk->m_ctx,
				static_cast<const unsigned char*>(m_src->get_base()),
				m_src->get_len(),
                static_cast<unsigned char*>(m_result->get_base()), &osize, m_result->get_len(),
                &random::read_func, m_random.get());
			m_result->set_len(osize);
		}
	};

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

	lua::multiret pk::encrypt(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("pk::encrypt is async");
			return {2};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("pk::encrypt operation in progress");
			return {2};
		}
		auto src = llae::buffer_base::get(l,2,true);
		if (!src) {
			l.argerror(2,"buffer expected");
			return {0};
		}
		auto random = lua::stack<random_ptr>::get(l,3);
		if (!random) {
			random = random_ptr(new crypto::random());
			random->randomize();
		}
		{
			common::intrusive_ptr<encrypt_async> req{new encrypt_async(pk_ptr(this),std::move(src),std::move(random))};
			
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

	void pk::on_completed(lua::state& l,int uvstatus,int mbedlsstatus,llae::buffer_base_ptr&& data) {
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

	void pk::lbind(lua::state& l) {
		lua::bind::function(l,"new",&pk::lnew);
		lua::bind::function(l,"parse_public_key",&pk::parse_public_key);
		lua::bind::function(l,"get_name",&pk::get_name);
		lua::bind::function(l,"get_rsa",&pk::get_rsa);
		lua::bind::function(l,"encrypt",&pk::encrypt);
	}

	lua::multiret pk::lnew(lua::state& l) {
		lua::push(l,pk_ptr(new pk()));
		return {1};
	}
}
