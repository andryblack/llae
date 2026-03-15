#ifndef __LLAE_CRYPTO_CRYPTO_H_INCLUDED__
#define __LLAE_CRYPTO_CRYPTO_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "lua/state.h"
#include "llae/error.h"
#include "llae/result.h"

namespace crypto {
	void push_error(lua::state& l,const char* fmt, int error);

	static inline lua::multiret mbedtls_result(lua::state& l,int res) {
		if (res == 0) {
			l.pushboolean(true);
			return {1};
		}
		l.pushnil();
		push_error(l,"failed code:%d, %s",res);
		return {2};
	}

	class status_error : public llae::code_error {
		META_OBJECT
    public:
    	static const std::string category;
    	explicit status_error(int status) : llae::code_error(status) {}
		virtual const std::string& get_category() const override { return category; }
    	virtual std::string to_string() const override;
		static llae::error_ptr create(int status) {
			return common::make_intrusive<status_error>(status);
		}
    };

	static inline llae::result<void> make_result(int status) {
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<void>{};
	}
}

#endif /*__LLAE_CRYPTO_CRYPTO_H_INCLUDED__*/
