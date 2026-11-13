#include "ecp.h"
#include "crypto.h"
#include "bignum.h"
#include "llae/buffer.h"
#include "random.h"

#include "llae-private/mbedtls/ecdsa.h"
#include "llae-private/mbedtls/ecdh.h"

#include <algorithm>
#include <cstring>
#include <string>


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

	llae::result<void> ecp_point::read_string(std::string_view x, std::string_view y,
			std::optional<int> radix) {
		std::string xs(x);
		std::string ys(y);
		auto status = mbedtls_ecp_point_read_string(&m_point,radix.value_or(10),
			xs.c_str(),ys.c_str());
		return make_result(status);
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

	llae::result<ecp_ptr> ecp::create(std::string_view group_id) {
		std::string name(group_id);
		auto info = mbedtls_ecp_curve_info_from_name(name.c_str());
		if (!info) {
			return llae::string_error::create("unknown ecp '" + name + "'");
		}
		ecp_ptr res(new ecp());
		auto status = res->load_group(info->grp_id);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<ecp_ptr>(std::move(res));
	}

	bool ecp::check_pubkey(const ecp_point_ptr& pnt) const {
		return pnt && mbedtls_ecp_check_pubkey(&m_group,pnt->get()) == 0;
	}

	bool ecp::check_privkey(const bignum_ptr& d) const {
		return d && mbedtls_ecp_check_privkey(&m_group,d->get()) == 0;
	}

	llae::result<ecp_point_ptr> ecp::point_read_binary(const llae::buffer_view& data) {
		ecp_point_ptr pnt = common::make_intrusive<ecp_point>();
		auto status = mbedtls_ecp_point_read_binary(&m_group,pnt->get(),
			static_cast<const unsigned char*>(data.get_base()),data.get_len());
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<ecp_point_ptr>(std::move(pnt));
	}

	llae::result<llae::buffer_base_ptr> ecp::point_write_binary(const ecp_point_ptr& point,
			std::optional<int> format) {
		if (!point) {
			return llae::string_error::create("need point");
		}
		int fmt = format.value_or(MBEDTLS_ECP_PF_COMPRESSED);
		unsigned char buf[2];
		size_t need_len = 0;
		auto status = mbedtls_ecp_point_write_binary(&m_group,point->get(),fmt,&need_len,buf,0);
		if (status != MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL) {
			return status_error::create(status);
		}
		auto b = llae::buffer::alloc(need_len);
		status = mbedtls_ecp_point_write_binary(&m_group,point->get(),fmt,&need_len,
			static_cast<unsigned char *>(b->get_base()),need_len);
		if (status != 0) {
			return status_error::create(status);
		}
		b->set_len(need_len);
		return llae::result<llae::buffer_base_ptr>(llae::buffer_base_ptr(std::move(b)));
	}

	llae::result<bignum_ptr> ecp::gen_privkey() {
		bignum_ptr d = common::make_intrusive<bignum>();
		auto status = mbedtls_ecp_gen_privkey(&m_group, d->get(), rng_func, this);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(d));
	}

	llae::result<ecp_point_ptr> ecp::gen_pubkey(const bignum_ptr& d) {
		if (!d) {
			return llae::string_error::create("need privkey");
		}
		ecp_point_ptr Q = common::make_intrusive<ecp_point>();
		auto status = mbedtls_ecp_mul( &m_group, Q->get(), d->get(), &m_group.G,
			&ecp::rng_func, this);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<ecp_point_ptr>(std::move(Q));
	}

	llae::result<ecp_keypair> ecp::gen_keypair() {
		ecp_keypair keypair{common::make_intrusive<bignum>(), common::make_intrusive<ecp_point>()};
		auto status = mbedtls_ecp_gen_keypair(&m_group,keypair.private_key->get(),
			keypair.public_key->get(),&ecp::rng_func,this);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<ecp_keypair>(std::move(keypair));
	}

	llae::result<bool> ecp::ecdsa_verify(const llae::buffer_view& hash, const ecp_point_ptr& Q,
			const bignum_ptr& r, const bignum_ptr& s) {
		if (!Q) {
			return llae::string_error::create("need pubkey");
		}
		if (!r) {
			return llae::string_error::create("need r");
		}
		if (!s) {
			return llae::string_error::create("need s");
		}
		auto res = mbedtls_ecdsa_verify(&m_group,
										static_cast<const unsigned char*>(hash.get_base()),
										hash.get_len(),Q->get(),r->get(),s->get());
		if (res != 0) {
			return status_error::create(res);
		}
		return true;
	}

	int ecp::rng_func(void *ctx, unsigned char * buf, size_t len) {
		return static_cast<ecp*>(ctx)->rng_gen(buf,len);
	}
	int ecp::rng_gen(unsigned char * buffer, size_t size) {
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
		if (size) {
			if (!m_random) {
				m_random = common::make_intrusive<random>(entropy_ptr{});
				auto r = m_random->seed(llae::buffer_view{});
				if (r.get_error()) {
					return -1;
				}
			}
			return m_random->read(buffer,size);
		}
		return 0;
	}

	void ecp::set_random_data(const llae::buffer_view& data) {
		if (data.empty()) {
			m_random_data.reset();
		} else {
			m_random_data = llae::buffer::hold(data.get_base(),data.get_len());
		}
	}

	llae::result<ecdsa_signature> ecp::ecdsa_sign(const llae::buffer_view& hash, const bignum_ptr& d,
			std::optional<std::string_view> hash_alg) {
		if (!d) {
			return llae::string_error::create("need privkey");
		}

		ecdsa_signature sig{common::make_intrusive<bignum>(), common::make_intrusive<bignum>()};

		int res;

#ifdef MBEDTLS_ECDSA_DETERMINISTIC
		if (hash_alg && !hash_alg->empty()) {
			std::string alg(*hash_alg);
			auto info = mbedtls_md_info_from_string(alg.c_str());
			if (!info) {
				return llae::string_error::create("unknown alg " + alg);
			}

			res = mbedtls_ecdsa_sign_det_ext(&m_group,sig.r->get(),sig.s->get(),d->get(),
									  static_cast<const unsigned char*>(hash.get_base()),
									  hash.get_len(),
									  mbedtls_md_get_type(info),
									  &ecp::rng_func,this);
		} else
#endif
		{
			res = mbedtls_ecdsa_sign(&m_group,sig.r->get(),sig.s->get(),d->get(),
									  static_cast<const unsigned char*>(hash.get_base()),
									  hash.get_len(),&ecp::rng_func,this);
		}
		if (res != 0) {
			return status_error::create(res);
		}
		return llae::result<ecdsa_signature>(std::move(sig));
	}


	llae::result<ecp_keypair> ecp::ecdh_gen_public() {
		ecp_keypair keypair{common::make_intrusive<bignum>(), common::make_intrusive<ecp_point>()};
		int status = mbedtls_ecdh_gen_public(&m_group,keypair.private_key->get(),
			keypair.public_key->get(),&ecp::rng_func,this);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<ecp_keypair>(std::move(keypair));
	}

	llae::result<bignum_ptr> ecp::ecdh_compute_shared(const ecp_point_ptr& Q, const bignum_ptr& d) {
		if (!Q) {
			return llae::string_error::create("need pubkey");
		}
		if (!d) {
			return llae::string_error::create("need privkey");
		}
		bignum_ptr z = common::make_intrusive<bignum>();
		int status = mbedtls_ecdh_compute_shared(&m_group, z->get(), Q->get(), d->get(),
			&ecp::rng_func,this);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(z));
	}


	llae::result<ecp_point_ptr> ecp::scalar_mul(const ecp_point_ptr& point, const bignum_ptr& d) {
		if (!point) {
			return llae::string_error::create("need point");
		}
		if (!d) {
			return llae::string_error::create("need scalar");
		}
		ecp_point_ptr R = common::make_intrusive<ecp_point>();
		int status = mbedtls_ecp_mul(&m_group, R->get(), d->get(), point->get(),
			&ecp::rng_func,this);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<ecp_point_ptr>(std::move(R));
	}

}
