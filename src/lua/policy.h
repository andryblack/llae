#pragma once
#include "stack.h"
#include <utility>
#include <cstring>
#include <algorithm>

namespace lua {

	namespace bind {

        enum class policy_type {
            field,
            func,
        };

        struct base_field_policy {
            static constexpr policy_type type = policy_type::field;
        };

        struct default_field_policy : base_field_policy {
            template <typename R>
			static int push_field(state& s,const R& result) {
				return stack<R>::push(s,result);
			}
			template <typename R>
			static void set_field(state& s, R& result, int idx) {
				result = stack<R>::get(s,idx);
			}
        };

        struct field_ref_policy : base_field_policy{
            template <typename R>
			static int push_field(state& s, R& result) {
				if (push_ptr(s,&result))
					ref_value(s,-1,1);
				return 1;
			}
			template <typename R>
			static void set_field(state& s, R& result, int value_idx) {
				result = stack<const R&>::get(s,value_idx);
			}
        };

        template <bool zero_terminate = true>
		struct string_policy : base_field_policy {
			template <size_t size>
			static int push_field(state& s,const char(&str)[size]) {
				auto zero_pos = std::find(str,str+size,0);
				if (zero_pos != str+size) {
					s.pushlstring(str,zero_pos-str);
				} else {
					s.pushlstring(str,size);
				}
				return 1;
			}
			template <typename T,size_t size>
			static void set_field(state& s,T(&str)[size],int value_idx) {
				size_t len = 0;
				if (auto ptr = s.checklstring(value_idx,len)) {
					if (len > size) {
						s.argerror(value_idx,"string too long");
					}
					std::memcpy(str,ptr,len);
					if (len < size) {
						std::memset(str + len,0,size - len);
					}
				}
			}
		};
		template <>
		struct string_policy<false> : string_policy<true> {
			template <typename T,size_t size>
			static int push_field(state& s,const T(&str)[size]) {
				s.pushlstring(reinterpret_cast<const char*>(str),size);
				return 1;
			}
		};

        struct base_func_policy {
            static constexpr policy_type type = policy_type::func;
            static constexpr bool has_return_policy = false;
        };
		struct default_func_policy : base_func_policy {
			
			template <typename R>
			static int push_result(state& s,R&& result) {
				return stack<R>::push(s,std::forward<R>(result));
			}
			
			template <size_t ArgIdx>
			struct arg_policy {
				template <typename Arg>
				using type = stack<Arg>;
			};
		};

		template <int idx = 1>
		struct return_ref_policy : base_func_policy {
			static constexpr bool has_return_policy = true;
			template <typename R>
			static int push_result(state& s,R&& result) {
				auto r = default_func_policy::push_result(s,std::forward<R>(result));
				ref_value(s,-r,idx);
				return r;
			}
			template <typename R>
			static int push_result(state& s,R* result) {
				if (push_ptr(s,result))
					ref_value(s,-1,idx);
				return 1;
			}
			template <typename R>
			static int push_result(state& s,R& result) {
				if (push_ptr(s,&result))
					ref_value(s,-1,idx);
				return 1;
			}
			
			template <size_t ArgIdx>
			using arg_policy = default_func_policy::template arg_policy<ArgIdx>;
		};
		using return_self_ref_policy = return_ref_policy<1>;

		template <int idx = 1>
		struct return_arg_policy : default_func_policy {
			static constexpr bool has_return_policy = true;
			template <typename R>
			static int push_result(state& s,R&&) {
				s.pushvalue(idx);
				return 1;
			}
		};
		using return_self_policy = return_arg_policy<1>;

		

	}
}