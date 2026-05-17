#ifndef __LLAE_UV_LUV_H_INCLUDED__
#define __LLAE_UV_LUV_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "llae/error.h"
#include "lua/state.h"
#include "decl.h"
#include "llae/result.h"
#include "llae/promise.h"
#include <string>
#include <vector>
#include "llae/buffer.h"
#include "lua/types.h"
#include "dns.h"

namespace llae {
	class loop;
}

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
	uv_loop_t* get_native(llae::loop& l);

	/// @luabind(name=exepath)
	llae::result<std::string> exepath();
	/// @luabind(name=cwd)
	llae::result<std::string> get_cwd();
	/// @luabind(name=chdir)
	llae::result<> lchdir(std::string_view dir);
	/// @luabind(name=gettimeofday)
	int lgettimeofday(lua_State* L);
	/// @luabind(name=interface_addresses)
	int linterface_addresses(lua_State* L);
	/// @luabind(name=set_process_title)
	llae::result<> lset_process_title(std::string_view title);
	
	/// @luabind(name=random)
	int lrandom(lua_State* L);
	/// @luabind(name=print_handles)
	int lprint_handles(lua_State* L);
	/// @luabind(name=available_parallelism,wrapper=uv_available_parallelism)
	int lavailable_parallelism(lua_State* L);
	/// @luabind(name=cpu_info)
	int lcpu_info(lua_State* L);
	/// @luabind(name=ip4_addr)
	lua::multiret lip4_addr(lua::state& l);
	/// @luabind(name=ip6_addr)
	lua::multiret lip6_addr(lua::state& l);
	/// @luabind(name=ip4_name)
	lua::multiret lip4_name(lua::state& l);
	/// @luabind(name=ip6_name)
	lua::multiret lip6_name(lua::state& l);
	/// @luabind(name=if_indextoname)
	lua::multiret lif_indextoname(lua::state& l);

#ifdef LUABIND_PARSE
	/// @luabind(value=AF_INET)
	LUABIND_FIELD(AF_INET)
	/// @luabind(value=AF_INET6)
	LUABIND_FIELD(AF_INET6)

	/// @luabind(wrapper=uv_get_free_memory)
	uint64_t get_free_memory();
	/// @luabind(wrapper=uv_get_total_memory)
	uint64_t get_total_memory();
	/// @luabind(wrapper=uv_get_constrained_memory)
	uint64_t get_constrained_memory();
	/// @luabind(wrapper=uv_get_available_memory)
	uint64_t get_available_memory();
	/// @luabind(wrapper=uv_hrtime)
	uint64_t hrtime();
	/// @luabind(wrapper=uv_sleep)
	void sleep(unsigned int ms);

	/// @luabind(async=true,wrapper=uv::timer_pause::pause)
	int pause(lua_State* L);
	/// @luabind(wrapper=uv::timer_delayed_resume::resume_delayed)
	int resume_delayed(lua_State* L);
	/// @luabind(async=true,wrapper=uv::getaddrinfo_req::async_getaddrinfo)
	llae::result_promise_ptr<std::vector<addrinfo_item>> getaddrinfo(llae::loop& l,std::string_view host,std::optional<std::string_view> service);

	/// @luabind(ltype=integer,value=UV_CLOCK_MONOTONIC)
	constexpr auto CLOCK_MONOTONIC = UV_CLOCK_MONOTONIC;
	/// @luabind(ltype=integer,value=UV_CLOCK_REALTIME)
	constexpr auto CLOCK_REALTIME = UV_CLOCK_REALTIME;
#endif


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

	template <typename R>
	class promise : public llae::promise<llae::result<R>> {
	public:
		using this_type = promise<R>;
		using this_ptr = common::intrusive_ptr<this_type>;
		using result_type = llae::result<R>;
	protected:
		void resolve_status(llae::loop& a,int status) {
			this->set_result(a,result_type(common::make_intrusive<status_error>(status)));
		}
	};
}

#endif /*__LLAE_UV_LUV_H_INCLUDED__*/
