#ifndef __LLAE_UV_LUV_H_INCLUDED__
#define __LLAE_UV_LUV_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "llae/error.h"
#include "lua/state.h"
#include "decl.h"
#include "llae/result.h"
#include <string>
#include <vector>
#include "llae/buffer.h"

namespace uv {

	void error(lua::state& l,int e);
	void error(int e,const char* file,int line);
	static inline void check_error(lua::state& l,int e) {
		if (e<0) error(l,e);
	}
#define UV_DIAG_CHECK(e) do{ if (e<0) ::uv::error(e,__FILE__,__LINE__); } while(false)
	void push_error(lua::state& s,int r);
	lua::multiret return_status_error(lua::state& s,int r);
	void print_error(int r);
	std::string get_error(int r);
    std::string get_cwd();


	static inline uv_buf_t get_buffer(const llae::buffer_base_ptr& buf) {
		if (!buf) {
			return uv_buf_init(nullptr,0);
		}
		auto base = const_cast<char*>(static_cast<const char*>(buf->get_base()));
		return uv_buf_init(base,static_cast<unsigned int>(buf->get_len()));
	}
	static inline uv_buf_t get_buffer(const llae::buffer_view& buf) {
		return uv_buf_init(const_cast<char*>(static_cast<const char*>(buf.get_base())),static_cast<unsigned int>(buf.get_len()));
	}
    static inline std::vector<uv_buf_t> get_buffers(const std::vector<llae::buffer_view>& buffers) {
        std::vector<uv_buf_t> res;
        res.reserve(buffers.size());
        for (auto& b:buffers) {
			res.emplace_back(get_buffer(b));
        }
        return res;
    }
	static llae::buffer_ptr get_buffer(const uv_buf_t* buf) {
		return llae::buffer_ptr(llae::buffer::get(buf->base));
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

    static inline llae::result<> make_result(int status) {
    	if (status < 0) {
    		return status_error::create(status);
    	} else {
    		return llae::result<>();
    	}
    }
}

#endif /*__LLAE_UV_LUV_H_INCLUDED__*/
