#ifndef __LLAE_UV_LUV_H_INCLUDED__
#define __LLAE_UV_LUV_H_INCLUDED__

#include "lua/state.h"
#include "decl.h"
#include <string>
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
			return uv_buf_t{nullptr,0};
		}
		auto base = const_cast<char*>(static_cast<const char*>(buf->get_base()));
		return uv_buf_t{base,buf->get_len()};
	}
	static llae::buffer_ptr get_buffer(const uv_buf_t* buf) {
		return llae::buffer_ptr(llae::buffer::get(buf->base));
	} 
}

#endif /*__LLAE_UV_LUV_H_INCLUDED__*/
