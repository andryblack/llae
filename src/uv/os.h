#ifndef __LLAE_UV_OS_H_INCLUDED__
#define __LLAE_UV_OS_H_INCLUDED__

#include "lua/state.h"
#include "llae/result.h"
#include "decl.h"
#include <unordered_map>
#include <string>

namespace uv {
	namespace os {
		/// @luabind
		llae::result<std::string> homedir();
		/// @luabind
		llae::result<std::string> tmpdir();
		/// @luabind
		llae::result<std::string> getenv(std::string_view name);
		/// @luabind
		llae::result<> setenv(std::string_view name, std::string_view value);
		/// @luabind
		llae::result<std::unordered_map<std::string,std::string>> getallenv();
		/// @luabind
		llae::result<> unsetenv(std::string_view name);
		/// @luabind
		llae::result<std::string> gethostname();
		/// @luabind
		llae::result<uv_utsname_t> uname();
		/// @luabind
		llae::result<int> getpriority(std::optional<int> pid);
		/// @luabind
		llae::result<> setpriority(int priority,std::optional<int> pid);
		/// @luabind
		int getpid();
	};
}

namespace lua {
	template <>
	struct stack<uv_utsname_t> {
		static int push(state& s,const uv_utsname_t& r);
	};
	template <>
	struct stack<const uv_utsname_t&> : stack<uv_utsname_t> {};
	template <>
	struct stack<std::unordered_map<std::string,std::string>> {
		static int push(state& s,const std::unordered_map<std::string,std::string>& r);
	};
	template <>
	struct stack<const std::unordered_map<std::string,std::string>&> : stack<std::unordered_map<std::string,std::string>> {};
}

#endif /*__LLAE_UV_OS_H_INCLUDED__*/