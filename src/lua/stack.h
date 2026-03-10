#pragma once

#include "state.h"
#include "value.h"
#include "meta/object.h"
#include "metatable.h"
#include "common/intrusive_ptr.h"
#include <cstdint>
#include <type_traits>
#include <string>
#include <string_view>
#include <optional>

namespace lua {

	template <class T, class enable = void>
	struct stack {};

    template <class T>
    struct check {
        using type = T;
    };

    template <class T>
    struct raw {
        using MT = T;
    };


	template <>
	struct stack<value> {
		static value get(state& s,int idx) {
			return value(s,idx);
		}
		static int push(state& s,const value& v) {
			v.push(s);
			return 1;
		}
	};

	template <typename T>
	struct stack<const T> : stack<T> {};

	template <>
	struct stack<char> {
		static char get(state& s,int idx) { return s.tointeger(idx); }
		static int push(state& s,char v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<unsigned char> {
		static unsigned char get(state& s,int idx) { return s.tointeger(idx); }
		static int push(state& s,unsigned char v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<signed char> {
		static signed char get(state& s,int idx) { return s.tointeger(idx); }
		static int push(state& s,signed char v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<short> {
		static short get(state& s,int idx) { return s.tointeger(idx); }
		static int push(state& s,short v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<unsigned short> {
		static unsigned short get(state& s,int idx) { return s.tointeger(idx); }
		static int push(state& s,unsigned short v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<int> {
		static int get(state& s,int idx) { return static_cast<int>(s.tointeger(idx)); }
		static int push(state& s,int v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<unsigned int> {
		static unsigned int get(state& s,int idx) { return static_cast<unsigned int>(s.tointeger(idx)); }
		static int push(state& s,unsigned int v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<long> {
		static long get(state& s,int idx) { return s.tointeger(idx); }
		static int push(state& s,long v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<unsigned long> {
		static unsigned long get(state& s,int idx) { return s.tointeger(idx); }
		static int push(state& s,unsigned long v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<long long> {
		static long long get(state& s,int idx) { return s.tointeger(idx); }
		static int push(state& s,long long v) { s.pushinteger(v); return 1; }
	};
	template <>
	struct stack<unsigned long long> {
		static unsigned long long get(state& s,int idx) { return s.tointeger(idx); }
		static int push(state& s,unsigned long long v) { s.pushinteger(v); return 1; }
	};
    
	template <>
	struct stack<const char*> {
		static const char* get(state& s,int idx) { return s.tostring(idx); }
		static int push(state& s,const char* v) { s.pushstring(v); return 1; }
	};
	template <>
	struct stack<float> {
		static float get(state& s,int idx) { return s.tonumber(idx); }
		static int push(state& s,float v) { s.pushnumber(v); return 1; }
	};
	template <>
	struct stack<double> {
		static double get(state& s,int idx) { return s.tonumber(idx); }
		static int push(state& s,double v) { s.pushnumber(v); return 1; }
	};
	template <>
	struct stack<bool> {
		static bool get(state& s,int idx) { return s.toboolean(idx); }
		static int push(state& s,bool v) { s.pushboolean(v); return 1; }
	};
	template <>
	struct stack<std::string> {
		static std::string get(state& s,int idx) { return s.checkstring(idx); }
		static int push(state& s,const std::string& v) { s.pushstring(v.c_str()); return 1; }
	};
	template <>
	struct stack<const std::string&> : stack<std::string> {};
	template <>
	struct stack<std::string_view> {
		static std::string_view get(state& s,int idx) { size_t len = 0; auto p = s.tolstring(idx,len); return {p,len}; }
		static int push(state& s,const std::string_view& v) { s.pushlstring(v.data(),v.size()); return 1; }
	};
	template <>
	struct stack<const std::string_view&> : stack<std::string_view> {};

	template <typename T>
	static inline common::intrusive_ptr<T> get_intrusive(state& l,int idx) {
		auto hdr = object_holder_t<T>::get(l,idx);
		if (!hdr) return common::intrusive_ptr<T>{};
		return hdr->template get_intrusive<T>();
	}
	template <typename T>
	static inline int push_intrusive(state& l,common::intrusive_ptr<T>&& v) {
		if (!v) {
			l.pushnil();
			return 1;
		}
		push_meta_object(l,std::move(v));
		return 1;
	}
	template <typename T>
	static inline int push_intrusive(state& l,const common::intrusive_ptr<T>& v) {
		if (!v) {
			l.pushnil();
			return 1;
		}
		push_meta_object(l,v);
		return 1;
	}
	template <class T>
	struct stack<common::intrusive_ptr<T> > {
		static common::intrusive_ptr<T> get(state& s,int idx) { 
			return get_intrusive<T>(s, idx);
		}
		static int push(state& s,common::intrusive_ptr<T>&& v) { 
			return push_intrusive(s, std::move(v));
		}
        static int push(state& s,const common::intrusive_ptr<T>& v) {
            return push_intrusive(s, v);
        }
	};
    template <class T>
    struct stack<check<common::intrusive_ptr<T> > > {
        static common::intrusive_ptr<T> get(state& s,int idx) {
        	auto hdr = object_holder_t<T>::get(s,idx);
            if (!hdr) {
                s.argerror(idx,meta::get_type_name<T>());
            }
            return hdr->template get_intrusive<T>();
        }
        static int push(state& s,common::intrusive_ptr<T>&& v) {
			return push_intrusive(s,std::move(v));
        }
        static int push(state& s,const common::intrusive_ptr<T>& v) {
            return push_intrusive(s,v);
        }
    };
	template <class T>
	struct stack<T*> {
		static T* get(state& s,int idx) {
			auto obj = meta_holder_base_t::get_ptr<T>(s,idx);
			if (obj.first && !std::is_const<T>::value && obj.second) {
				s.error("invalid pointer %s is const",meta::info<T>::get()->name);
			}
            return obj.first;
		}
	};
	template <class T>
	struct stack<T&> {
		static T& get(state& s,int idx) {
			auto obj = meta_holder_base_t::get_ptr<T>(s,idx);
			if (!obj.first) {
				s.error("invalid reference %s",meta::info<T>::get()->name);
			}
			if (!std::is_const<T>::value && obj.second) {
				s.error("invalid pointer %s is const",meta::info<T>::get()->name);
			}
            return *obj.first;
		}
	};

    template <class T>
    struct stack<T,typename std::enable_if< std::is_enum<T>::value>::type> {
        static T get(state& s,int idx) {
            return static_cast<T>(stack<int>::get(s,idx));
        }
        static int push(state& s,T v) {
            return stack<int>::push(s,int(v));
        }
    };

//    template <class T>
//    struct stack<T&&,typename std::enable_if< std::is_copy_constructible<T>::value>::type> {
//        static void push(state& s,T&& v) {
//            push_raw(s,std::forward<T>(v));
//        }
//    };

	template <class T>
	struct stack<const common::intrusive_ptr<T>& > : stack<common::intrusive_ptr<T> >{};
	template <class T>
    struct stack<common::intrusive_ptr<T>&& > : stack<common::intrusive_ptr<T> >{};


    template <class MT>
    struct stack<raw<MT>> {
    	using T = typename MT::object;
    	static T* get(state& s,int idx) {
    		auto res = s.testudata(idx,MT::name);
    		return static_cast<T*>(res);
        }
        static int push(state& s,const T& v) {
            auto ptr = s.newuserdata(sizeof(T));
            memcpy(ptr,&v,sizeof(T));
            s.setmetatable(MT::name);
            return 1;
        }
    };

    template <class MT>
    struct stack<check<raw<MT>>> : stack<raw<MT>> {
    	using T = typename MT::object;
    	static T* get(state& s,int idx) {
    		auto res = s.checkudata(idx,MT::name);
    		return static_cast<T*>(res);
        }
    };

    template <class T>
    struct stack<std::optional<T>> {
    	using up = stack<T>;
    	static std::optional<T> get(state& s,int idx) {
    		if (s.isnoneornil(idx)) {
    			return {};
    		}
    		return up::get(s,idx);
    	}
    	static int push(state& s,const std::optional<T>& v) {
            if (!v) {
            	s.pushnil();
				return 1;
            } else {
            	return up::push(s,*v);
            }
        }
        static int push(state& s, std::optional<T>&& v) {
            if (!v) {
            	s.pushnil();
				return 1;
            } else {
				return up::push(s,std::move(*v));
            }
        }
    };

	
    template <class T>
    static int push(state& s,const T& val) {
        return stack<T>::push(s,val);
    }
    template <class T>
    static int push(state& s,T& val) {
        return stack<T>::push(s,val);
    }
    template <class T>
    static int push(state& s, T&& val) {
        return stack<T>::push(s,std::forward<T>(val));
    }
}
