#include "bignum.h"
#include "crypto.h"
#include "random.h"
#include "llae/buffer.h"
#include <cstring>
#include <string>

META_OBJECT_INFO(crypto::bignum,meta::object)

namespace crypto {

	namespace {
		/// Multiplies an MPI by a signed integer without a temporary bignum.
		llae::error_ptr mul_int(mbedtls_mpi& x,const mbedtls_mpi& a,int64_t b) {
			if (b >= 0) {
				auto status = mbedtls_mpi_mul_int(&x,&a,static_cast<mbedtls_mpi_uint>(b));
				return status != 0 ? status_error::create(status) : llae::error_ptr{};
			}
			mbedtls_mpi abs_value;
			mbedtls_mpi_init(&abs_value);
			mbedtls_mpi zero;
			mbedtls_mpi_init(&zero);
			auto status = mbedtls_mpi_mul_int(&abs_value,&a,
				static_cast<mbedtls_mpi_uint>(-(b+1))+1);
			if (status == 0) {
				status = mbedtls_mpi_sub_mpi(&x,&zero,&abs_value);
			}
			mbedtls_mpi_free(&abs_value);
			mbedtls_mpi_free(&zero);
			return status != 0 ? status_error::create(status) : llae::error_ptr{};
		}
	}

	bignum::bignum() {
		mbedtls_mpi_init(&m_mpi);
	}

	bignum::~bignum() {
		mbedtls_mpi_free(&m_mpi);
	}

	void bignum::set(mbedtls_mpi_sint val) {
		mbedtls_mpi_lset(&m_mpi, val);
	}
	void bignum::set(const bignum& val) {
		mbedtls_mpi_copy(&m_mpi, &val.m_mpi);
	}

	llae::result<bignum_ptr> bignum::create(std::optional<int64_t> value) {
		bignum_ptr ptr = common::make_intrusive<bignum>();
		if (value) {
			std::string str(std::to_string(*value));
			auto status = mbedtls_mpi_read_string(&ptr->m_mpi, 10, str.c_str());
			if (status != 0) {
				return status_error::create(status);
			}
		}
		return llae::result<bignum_ptr>(std::move(ptr));
	}

	llae::result<bignum_ptr> bignum::from_string(std::string_view value, std::optional<int> radix) {
		std::string str(value);
		bignum_ptr ptr = common::make_intrusive<bignum>();
		auto status = mbedtls_mpi_read_string(&ptr->m_mpi, radix.value_or(16), str.c_str());
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(ptr));
	}

	llae::result<bignum_ptr> bignum::create_random(const random_ptr& rng, size_t len) {
		if (!rng) {
			return llae::string_error::create("need random");
		}
		bignum_ptr ptr = common::make_intrusive<bignum>();
		auto status = mbedtls_mpi_fill_random(&ptr->m_mpi, len, crypto::random::read_func, rng.get());
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(ptr));
	}

	bool bignum::is0() const {
		return mbedtls_mpi_cmp_int(&m_mpi,0) == 0;
	}

	bool bignum::get_bit(size_t bit) const {
		return mbedtls_mpi_get_bit(&m_mpi,bit);
	}

	llae::result<void> bignum::set_bit(size_t bit, bool value) {
		return make_result(mbedtls_mpi_set_bit(&m_mpi,bit,value ? 1 : 0));
	}

	llae::result<void> bignum::self_add(const bignum_ptr& other) {
		if (!other) {
			return llae::string_error::create("need value");
		}
		return make_result(mbedtls_mpi_add_mpi(&m_mpi, &m_mpi, &other->m_mpi));
	}

	llae::result<void> bignum::self_add(int64_t other) {
		return make_result(mbedtls_mpi_add_int(&m_mpi,&m_mpi,other));
	}

	llae::result<void> bignum::self_mul(const bignum_ptr& other) {
		if (!other) {
			return llae::string_error::create("need value");
		}
		return make_result(mbedtls_mpi_mul_mpi(&m_mpi, &m_mpi, &other->m_mpi));
	}

	llae::result<void> bignum::self_mul(int64_t other) {
		if (auto e = mul_int(m_mpi,m_mpi,other)) {
			return std::move(e);
		}
		return llae::result<void>{};
	}

	llae::result<void> bignum::self_sub(const bignum_ptr& other) {
		if (!other) {
			return llae::string_error::create("need value");
		}
		return make_result(mbedtls_mpi_sub_mpi(&m_mpi, &m_mpi, &other->m_mpi));
	}

	llae::result<void> bignum::self_sub(int64_t other) {
		return make_result(mbedtls_mpi_sub_int(&m_mpi,&m_mpi,other));
	}

	llae::result<void> bignum::self_lshift(size_t shift) {
		return make_result(mbedtls_mpi_shift_l(&m_mpi,shift));
	}

	llae::result<void> bignum::self_rshift(size_t shift) {
		return make_result(mbedtls_mpi_shift_r(&m_mpi,shift));
	}

	llae::result<bignum_ptr> bignum::mul(const bignum_ptr& other) {
		if (!other) {
			return llae::string_error::create("need value");
		}
		bignum_ptr res = common::make_intrusive<bignum>();
		auto status = mbedtls_mpi_mul_mpi(&res->m_mpi, &m_mpi, &other->m_mpi);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(res));
	}

	llae::result<bignum_ptr> bignum::mul(int64_t other) {
		bignum_ptr res = common::make_intrusive<bignum>();
		if (auto e = mul_int(res->m_mpi,m_mpi,other)) {
			return std::move(e);
		}
		return llae::result<bignum_ptr>(std::move(res));
	}

	llae::result<bignum_div_result> bignum::div(const bignum_ptr& other) {
		if (!other) {
			return llae::string_error::create("need value");
		}
		bignum_div_result res{common::make_intrusive<bignum>(), common::make_intrusive<bignum>()};
		auto status = mbedtls_mpi_div_mpi(&res.quotient->m_mpi, &res.remainder->m_mpi,
			&m_mpi, &other->m_mpi);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_div_result>(std::move(res));
	}

	llae::result<bignum_div_result> bignum::div(int64_t other) {
		bignum_div_result res{common::make_intrusive<bignum>(), common::make_intrusive<bignum>()};
		auto status = mbedtls_mpi_div_int(&res.quotient->m_mpi, &res.remainder->m_mpi,
			&m_mpi, other);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_div_result>(std::move(res));
	}

	llae::result<bignum_ptr> bignum::add(const bignum_ptr& other) {
		if (!other) {
			return llae::string_error::create("need value");
		}
		bignum_ptr res = common::make_intrusive<bignum>();
		auto status = mbedtls_mpi_add_mpi(&res->m_mpi, &m_mpi, &other->m_mpi);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(res));
	}

	llae::result<bignum_ptr> bignum::add(int64_t other) {
		bignum_ptr res = common::make_intrusive<bignum>();
		auto status = mbedtls_mpi_add_int(&res->m_mpi, &m_mpi, other);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(res));
	}

	llae::result<bignum_ptr> bignum::sub(const bignum_ptr& other) {
		if (!other) {
			return llae::string_error::create("need value");
		}
		bignum_ptr res = common::make_intrusive<bignum>();
		auto status = mbedtls_mpi_sub_mpi(&res->m_mpi, &m_mpi, &other->m_mpi);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(res));
	}

	llae::result<bignum_ptr> bignum::sub(int64_t other) {
		bignum_ptr res = common::make_intrusive<bignum>();
		auto status = mbedtls_mpi_sub_int(&res->m_mpi, &m_mpi, other);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(res));
	}

	llae::result<bignum_ptr> bignum::mod(const bignum_ptr& other) {
		if (!other) {
			return llae::string_error::create("need value");
		}
		bignum_ptr res = common::make_intrusive<bignum>();
		auto status = mbedtls_mpi_mod_mpi(&res->m_mpi, &m_mpi, &other->m_mpi);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(res));
	}

	llae::result<bignum_ptr> bignum::mod(int64_t other) {
		mbedtls_mpi_uint remainder = 0;
		auto status = mbedtls_mpi_mod_int(&remainder, &m_mpi, other);
		if (status != 0) {
			return status_error::create(status);
		}
		bignum_ptr res = common::make_intrusive<bignum>();
		res->set(static_cast<mbedtls_mpi_sint>(remainder));
		return llae::result<bignum_ptr>(std::move(res));
	}

	llae::result<bignum_ptr> bignum::exp_mod(const bignum_ptr& e, const bignum_ptr& n, const bignum_ptr& supp) {
		if (!e) {
			return llae::string_error::create("need E value");
		}
		if (!n) {
			return llae::string_error::create("need N value");
		}
		bignum_ptr res = common::make_intrusive<bignum>();
		auto status = mbedtls_mpi_exp_mod(res->get(), &m_mpi, e->get(), n->get(),
			supp ? supp->get() : nullptr);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<bignum_ptr>(std::move(res));
	}

	llae::result<std::string> bignum::to_string(std::optional<int> radix) const {
		int base = radix.value_or(16);
		char dummy = 0;
		size_t len = 0;
		auto status = mbedtls_mpi_write_string(&m_mpi,base,&dummy,0,&len);
		if (status != MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL) {
			return status_error::create(status);
		}
		std::string buffer(len,'\0');
		status = mbedtls_mpi_write_string(&m_mpi,base,buffer.data(),buffer.size(),&len);
		if (status != 0) {
			return status_error::create(status);
		}
		buffer.resize(std::strlen(buffer.c_str()));
		return buffer;
	}

	llae::result<llae::buffer_base_ptr> bignum::write(std::optional<size_t> length) const {
		size_t len = length.value_or(mbedtls_mpi_size(&m_mpi));
		auto buffer = llae::buffer::alloc(len);
		auto status = mbedtls_mpi_write_binary(&m_mpi,
			static_cast<unsigned char*>(buffer->get_base()),len);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<llae::buffer_base_ptr>(llae::buffer_base_ptr(std::move(buffer)));
	}

	llae::result<llae::buffer_base_ptr> bignum::write_le(std::optional<size_t> length) const {
		size_t len = length.value_or(mbedtls_mpi_size(&m_mpi));
		auto buffer = llae::buffer::alloc(len);
		auto status = mbedtls_mpi_write_binary_le(&m_mpi,
			static_cast<unsigned char*>(buffer->get_base()),len);
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<llae::buffer_base_ptr>(llae::buffer_base_ptr(std::move(buffer)));
	}

	llae::result<void> bignum::read(const llae::buffer_view& data) {
		auto status = mbedtls_mpi_read_binary(&m_mpi,
			static_cast<const unsigned char*>(data.get_base()),data.get_len());
		return make_result(status);
	}

	llae::result<void> bignum::read_le(const llae::buffer_view& data) {
		auto status = mbedtls_mpi_read_binary_le(&m_mpi,
			static_cast<const unsigned char*>(data.get_base()),data.get_len());
		return make_result(status);
	}

	bool bignum::less(const bignum_ptr& other) const {
		return other && mbedtls_mpi_cmp_mpi(&m_mpi,&other->m_mpi) == -1;
	}

	bool bignum::less(int64_t other) const {
		return mbedtls_mpi_cmp_int(&m_mpi,other) == -1;
	}

	bool bignum::lequal(const bignum_ptr& other) const {
		if (!other) {
			return false;
		}
		auto v = mbedtls_mpi_cmp_mpi(&m_mpi,&other->m_mpi);
		return v == -1 || v == 0;
	}

	bool bignum::lequal(int64_t other) const {
		auto v = mbedtls_mpi_cmp_int(&m_mpi,other);
		return v == -1 || v == 0;
	}

	bool bignum::equal(const bignum_ptr& other) const {
		return other && mbedtls_mpi_cmp_mpi(&m_mpi,&other->m_mpi) == 0;
	}

	bool bignum::equal(int64_t other) const {
		return mbedtls_mpi_cmp_int(&m_mpi,other) == 0;
	}
}
