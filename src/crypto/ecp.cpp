#include "ecp.h"
#include "crypto.h"
#include "bignum.h"
#include "lua/bind.h"
#include "uv/buffer.h"
#include "lua/stack.h"

#include <mbedtls/ecdsa.h>

META_OBJECT_INFO(crypto::ecp,meta::object)
META_OBJECT_INFO(crypto::ecp_point,meta::object)

namespace crypto {

	ecp_point::ecp_point() {
		mbedtls_ecp_point_init(&m_point);
	}

	ecp_point::~ecp_point() {
		mbedtls_ecp_point_free(&m_point);
	}

	void ecp_point::set_zero() {
		mbedtls_ecp_set_zero(&m_point);
	}

	bool ecp_point::is_zero() {
		return mbedtls_ecp_is_zero(&m_point);
	}

	bool ecp_point::cmp(const ecp_point_ptr& pnt) const {
		return pnt && (mbedtls_ecp_point_cmp(&m_point,pnt->get())) == 0;
	}

	lua::multiret ecp_point::read_string(lua::state& l) {
		const char* x = l.checkstring(2);
		const char* y = l.checkstring(3);
		int radix = l.optinteger(4,10);
		auto r = mbedtls_ecp_point_read_string(&m_point,radix,x,y);
		if (r == 0) {
			l.pushboolean(1);
			return {1};
		}
		l.pushnil();
        push_error(l,"read_string failed, code:%d, %s",r);
        return {2};
	}

	void ecp_point::lbind(lua::state& l) {
		lua::bind::function(l,"new",&ecp_point::lnew);
		lua::bind::function(l,"set_zero",&ecp_point::set_zero);
		lua::bind::function(l,"is_zero",&ecp_point::is_zero);
		lua::bind::function(l,"cmp",&ecp_point::cmp);
		lua::bind::function(l,"read_string",&ecp_point::read_string);
	}

	lua::multiret ecp_point::lnew(lua::state& l) {
		lua::push(l,ecp_point_ptr(new ecp_point()));
		return {1};
	}

	ecp::ecp() {
		mbedtls_ecp_group_init(&m_group);
	}
	ecp::~ecp() {
		mbedtls_ecp_group_free(&m_group);
	}

	int ecp::load_group(mbedtls_ecp_group_id grp_id) {
		return mbedtls_ecp_group_load(&m_group,grp_id);
	}

	bool ecp::check_pubkey(const ecp_point_ptr& pnt) const {
		return pnt && mbedtls_ecp_check_pubkey(&m_group,pnt->get()) == 0;
	}

    bool ecp::check_privkey(const bignum_ptr& d) const {
        return d && mbedtls_ecp_check_privkey(&m_group,d->get()) == 0;
    }

    lua::multiret ecp::point_read_binary(lua::state& l) {
        ecp_point_ptr pnt(new ecp_point());
        const unsigned char* data = nullptr;
        size_t size = 0;
        if (l.isstring(2)) {
            data = reinterpret_cast<const unsigned char*>(l.tolstring(2, size));
        } else {
            uv::buffer_ptr b = lua::stack<uv::buffer_ptr>::get(l,2);
            if (!b) {
                l.argerror(2, "need data");
            } else {
                data = static_cast<const unsigned char*>(b->get_base());
                size = b->get_len();
            }
        }
        auto r = mbedtls_ecp_point_read_binary(&m_group,pnt->get(),data,size);
        if (r == 0) {
            lua::push(l, std::move(pnt));
            return {1};
        }
        l.pushnil();
        push_error(l,"point_read_binary failed, code:%d, %s",r);
        return {2};
    }

    lua::multiret ecp::point_write_binary(lua::state& l) {
        ecp_point_ptr point = lua::stack<ecp_point_ptr>::get(l, 2);
        if (!point) {
            l.argerror(2, "need point");
        }
        int fmt = l.optinteger(3, MBEDTLS_ECP_PF_COMPRESSED);
        unsigned char buf[2];
        size_t need_len = 0;
        auto res = mbedtls_ecp_point_write_binary(&m_group,point->get(),fmt,&need_len,buf,0);
        if (res == MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL) {
            uv::buffer_ptr b = uv::buffer::alloc(need_len);
            res = mbedtls_ecp_point_write_binary(&m_group,point->get(),fmt,&need_len,
                                                 static_cast<unsigned char *>(b->get_base()),need_len);
            if (res == 0) {
                b->set_len(need_len);
                lua::push(l, std::move(b));
                return {1};
            }
        }
        l.pushnil();
        push_error(l,"point_write_binary failed, code:%d, %s",res);
        return {2};
    }

    lua::multiret ecp::gen_privkey(lua::state& l) {
        bignum_ptr d(new bignum());
        auto r = mbedtls_ecp_gen_privkey(&m_group, d->get(), rng_func, this);
        if (r == 0) {
            lua::push(l, std::move(d));
            return {2};
        }
        l.pushnil();
        push_error(l,"gen_privkey failed, code:%d, %s",r);
        return {2};
    }

    lua::multiret ecp::gen_pubkey(lua::state& l) {
        bignum_ptr d = lua::stack<bignum_ptr>::get(l,2);
        if (!d) {
            l.argerror(2, "need privkey");
        }
        ecp_point_ptr Q(new ecp_point());
        auto r = mbedtls_ecp_mul( &m_group, Q->get(), d->get(), &m_group.G, &ecp::rng_func, this);
        if (r == 0) {
            lua::push(l, std::move(Q));
            return {2};
        }
        l.pushnil();
        push_error(l,"gen_pubkey failed, code:%d, %s",r);
        return {2};
    }

    lua::multiret ecp::gen_keypair(lua::state& l) {
        bignum_ptr d(new bignum());
        ecp_point_ptr Q(new ecp_point());
        auto r = mbedtls_ecp_gen_keypair(&m_group,d->get(),Q->get(),&ecp::rng_func,this);
        if (r == 0) {
            lua::push(l, std::move(d));
            lua::push(l, std::move(Q));
            return {2};
        }
        l.pushnil();
        push_error(l,"gen_keypair failed, code:%d, %s",r);
        return {2};
    }

    lua::multiret ecp::ecdsa_verify(lua::state& l) {
        uv::buffer_ptr b = uv::buffer::get(l,2,true);
        if (!b) {
            l.argerror(2, "need hased data");
        }
        ecp_point_ptr Q = lua::stack<ecp_point_ptr>::get(l,3);
        if (!Q) {
            l.argerror(3, "need pubkey");
        }
        bignum_ptr r = lua::stack<bignum_ptr>::get(l,4);
        if (!r) {
            l.argerror(4, "need r");
        }
        bignum_ptr s = lua::stack<bignum_ptr>::get(l,5);
        if (!s) {
            l.argerror(4, "need s");
        }
        auto res = mbedtls_ecdsa_verify(&m_group,
                                        static_cast<const unsigned char*>(b->get_base()),
                                        b->get_len(),Q->get(),r->get(),s->get());
        if (res == 0) {
            l.pushboolean(true);
            return {1};
        }
        l.pushnil();
        push_error(l,"ecdsa_verify failed, code:%d, %s",res);
        return {2};
    }

    int ecp::rng_func(void *ctx, unsigned char * buf, size_t len) {
        static_cast<ecp*>(ctx)->rng_gen(buf,len);
        return 0;
    }
    void ecp::rng_gen(unsigned char * buffer, size_t size) {
        if (m_random_data && size) {
            size_t len = std::min(size,m_random_data->get_len());
            memcpy(buffer,static_cast<const char*>(m_random_data->get_base())+m_random_data->get_len()-len,len);
            buffer += len;
            size -= len;
            m_random_data->set_len(m_random_data->get_len()-len);
            if (m_random_data->get_len() == 0) {
                m_random_data.reset();
            }
        }
        for (size_t i=0;i<size;++i) {
            buffer[i] = rand();
        }
    }

    lua::multiret ecp::set_random_data(lua::state& l) {
        uv::buffer_ptr b = uv::buffer::get(l,2,true);
        if (!b) {
            l.argerror(2, "need hased data");
        }
        m_random_data = b;
        return {0};
    }

    lua::multiret ecp::ecdsa_sign(lua::state& l) {
        uv::buffer_ptr b = uv::buffer::get(l,2,true);
        if (!b) {
            l.argerror(2, "need hased data");
        }
        bignum_ptr d = lua::stack<bignum_ptr>::get(l,3);
        if (!d) {
            l.argerror(3, "need privkey");
        }
        
        bignum_ptr r(new bignum());
        bignum_ptr s(new bignum());
        
        int res;
        
#ifdef MBEDTLS_ECDSA_DETERMINISTIC
        const char* hash_alg = l.optstring(4, nullptr);
        if (hash_alg) {
            auto info = mbedtls_md_info_from_string(hash_alg);
            if (!info) {
                l.pushnil();
                l.pushfstring("unknown alg %s", hash_alg);
                return {2};
            }
            
            res = mbedtls_ecdsa_sign_det_ext(&m_group,r->get(),s->get(),d->get(),
                                      static_cast<const unsigned char*>(b->get_base()),
                                      b->get_len(),
                                      mbedtls_md_get_type(info),
                                      &ecp::rng_func,this);
        } else
#endif
        {
            res = mbedtls_ecdsa_sign(&m_group,r->get(),s->get(),d->get(),
                                      static_cast<const unsigned char*>(b->get_base()),
                                      b->get_len(),&ecp::rng_func,this);
        }
        if (res == 0) {
            lua::push(l, std::move(r));
            lua::push(l, std::move(s));
            return {2};
        }
        l.pushnil();
        push_error(l,"ecdsa_sign failed, code:%d, %s",res);
        return {2};
    }

	void ecp::lbind(lua::state& l) {
		lua::bind::function(l,"new",&ecp::lnew);
		lua::bind::function(l,"check_pubkey",&ecp::check_pubkey);
        lua::bind::function(l,"check_privkey",&ecp::check_privkey);
        lua::bind::function(l,"point_read_binary",&ecp::point_read_binary);
        lua::bind::function(l,"point_write_binary",&ecp::point_write_binary);
        lua::bind::function(l,"ecdsa_verify",&ecp::ecdsa_verify);
        lua::bind::function(l,"ecdsa_sign",&ecp::ecdsa_sign);
        lua::bind::function(l,"gen_privkey",&ecp::gen_privkey);
        lua::bind::function(l,"gen_pubkey",&ecp::gen_pubkey);
        lua::bind::function(l,"gen_keypair",&ecp::gen_keypair);
        lua::bind::function(l,"set_random_data",&ecp::set_random_data);
	}

	lua::multiret ecp::lnew(lua::state& l) {
		const char* name = l.checkstring(1);
		auto info = mbedtls_ecp_curve_info_from_name(name);
		if (!info) {
			l.pushnil();
			l.pushfstring("unknown ecp '%s'",name);
			return {2};
		}
		ecp_ptr res(new ecp());
		auto mbedlsstatus = res->load_group(info->grp_id);
		if (mbedlsstatus!=0) {
			l.pushnil();
            push_error(l,"group load failed, code:%d, %s",mbedlsstatus);
            return {2};
		}
		lua::push(l,std::move(res));
		return {1};
	}

}
