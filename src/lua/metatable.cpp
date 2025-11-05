#include "metatable.h"
#include "stack.h"
#include "bind.h"

namespace lua {

	static const constexpr lua_Integer metaobject__info = 1;
	static const constexpr lua_Integer metaobject__getters = 2;
	static const constexpr lua_Integer metaobject__setters = 3;

	struct auto_pop {
		state& s;
		int n;
		auto_pop(state& s,int n) : s(s), n(n) {
		}
		~auto_pop() {
			s.pop(n);
		}
	};

	struct field_t {
		lua_CFunction get;
		lua_CFunction set;
	};

	struct check_stack {
		state& s;
		int n;
		check_stack(state& s,int n) : s(s), n(s.gettop()+n) {
		}
		~check_stack() {
			assert(s.gettop() == n);
		}
	};

	static void get_metaobject_field_metatable(state& s) {
		auto t = s.getmetatable("metaobject_field");
		if (t != value_type::table) {
			s.pop(1);
			s.newmetatable("metaobject_field");
		}
	}

	static void set_metaobject_field_metatable(state& s) {
        check_stack cs(s, 0);
		get_metaobject_field_metatable(s);
		s.setmetatable(-2);
	}

	static bool is_metaobject_field(state& s,int idx) {
		if (!s.getmetatable(idx)) {
			return false;
		}
		auto_pop pop(s,2);
		get_metaobject_field_metatable(s);
		return s.rawequal(-1,-2);
	}

	static int metaobject_destroy(lua_State* L) {
		state s(L);
		auto hdr = meta_holder_base_t::get(s,1);
		if (hdr) {
			hdr->~meta_holder_base_t();
		}
		return 0;
	}

	static int metaobject_tostring(lua_State* L) {
		state s(L);
		auto hdr = meta_holder_base_t::get(s,1);
		if (hdr) {
			s.pushstring(hdr->info()->name);
		} else {
			s.pushstring("unknown");
		}
		return 1;
	}
	static int metaobject_reset_ref(lua_State* L) {
		state s(L);
		auto hdr = object_holder_base_t::get(s,1);
		if (hdr) {
			hdr->release();
		}
		return 0;
	}

	static const meta::info_t* get_meta_info(state& s,int idx) {
        if (!s.getmetatable(idx)) {
            return nullptr;
        }
		auto_pop pop(s,2);
		if (s.rawgeti(-1,metaobject__info) == value_type::lightuserdata) {
			return static_cast<const meta::info_t*>(s.touserdata(-1));
		}
		return nullptr;
	}

    static const char* get_meta_name(state& s,int idx) {
        auto info = get_meta_info(s,idx);
        return info ? info->name : "unknown";
    }

	static int metaobject__index(lua_State* L) {
		state s(L);
        check_stack cs(s, 1);
        if (!s.getmetatable(1)) {
			s.pushnil();
            return 1;
        }
		s.pushvalue(2);
		auto t = s.rawget(-2);
        s.remove(-2);
		if (t == value_type::table && is_metaobject_field(s,-1)) {
			if (s.rawgeti(-1,metaobject__getters) == value_type::function) {
				s.remove(-2);
				s.pushvalue(1);
				s.call(1,1);
				return 1;
			}
		}
		return 1;
	}
	static int metaobject__newindex(lua_State* L) {
		state s(L);
        check_stack cs(s, 0);
        if (!s.getmetatable(1)) {
            s.error("invalid target for __newindex");
            return 0;
        }
        s.pushvalue(2);
		auto t = s.rawget(-2);
        s.remove(-2);
		if (t == value_type::table && is_metaobject_field(s,-1)) {
			if (s.rawgeti(-1,metaobject__setters) == value_type::function) {
				s.remove(-2);
				s.pushvalue(1);
				s.pushvalue(3);
				s.call(2,0);
				return 0;
			} else {
				s.pop(1);
				s.error("set read only field %s at %s",s.tostring(2),get_meta_name(s,1));
				return 0;
			}
		}
		s.pop(1);
		s.error("set unknown field %s at %s",s.tostring(2),get_meta_name(s,1));
		return 0;
	}

	static void metaobject_bind(state& s) {
		bind::function(s,"free",&metaobject_reset_ref);
	}

	void register_meta_object_metatable(state& s) {
        bind::object<meta::object>::register_metatable(s,&metaobject_bind);
	}

	static void set_parent_metatable_fields(state& s,const meta::info_t* parent,lua_Integer field) {
		if (parent) {
			if (s.getmetatable(parent->name) != value_type::table) {
				s.pop(1);
				return;
			}
			if (s.rawgeti(-1,field) != value_type::table) {
				s.pop(2);
				return;
			}
			s.setfield(-3,"__index");
			s.pop(1);
			s.pushvalue(-1);
			s.setmetatable(-2);
		}
	}
	void create_metatable(state& s,const meta::info_t* info) {
        check_stack sc(s,1);
		if (!s.newmetatable(info->name)) {
			s.pop(1);
			s.error("metatable %s already registered",info->name);
		}
		const meta::info_t* parent = info->parent;
		if (parent) {
            if (s.getmetatable(parent->name) != value_type::table ) {
                s.error("unregistered parent metatable %s",parent->name);
            }
            s.pushnil();
			while (s.next(-2)) { // mt,p-mt,k,v
				s.pushvalue(-2); // mt,p-mt,k,v,k
				s.pushvalue(-2); // mt,p-mt,k,v,k,v 
				s.rawset(-6); // mt,p-mt,k,v
				s.pop(1); // mt,p-mt,k
			}
			s.pop(1);
        }
		
		s.pushlightuserdata(const_cast<meta::info_t*>(info)); // mt, info
		s.rawseti(-2,metaobject__info);
		s.pushcclosure(&metaobject__index,0);
		s.setfield(-2,"__index");
		s.pushcclosure(&metaobject__newindex,0);
		s.setfield(-2,"__newindex");
		s.pushcclosure(&metaobject_destroy,0);
		s.setfield(-2,"__gc");
		s.pushcclosure(&metaobject_tostring,0);
		s.setfield(-2,"__tostring");
        
	}

	void set_metatable(state& s,const meta::info_t* info) {
		if (s.getmetatable(info->name) == value_type::lnil) {
			s.error("not registered metatable %s",info->name);
		}
		s.setmetatable(-2);
	}
	void get_metatable(state& s,const meta::info_t* info) {
		s.getmetatable(info->name);
	}

	void metatable_set_setter(state& s, const char* name, int mtidx) {
		check_stack cs(s,-1);
        if (mtidx < 0) {
            mtidx = s.gettop()+mtidx+1;
        }
        if (!s.istable(mtidx)) {
            s.error("need metatable");
        }
        s.pushstring(name);
		auto t = s.rawget(mtidx);
		if (t == value_type::none || t == value_type::lnil) {
			s.pop(1);
            s.pushstring(name);
            s.createtable(0,0);
			s.pushvalue(-3);
			s.seti(-2,metaobject__setters);
			set_metaobject_field_metatable(s);
			s.rawset(mtidx);
            s.pop(1);
			return;
		} else if (t == value_type::table) {
			s.pushvalue(-2);
			s.seti(-2,metaobject__setters);
            s.pop(2);
            return;
		}
        s.pop(2);
        s.error("override field %s on %s",name,s.tostring(mtidx));
	}

	void metatable_set_getter(state& s, const char* name, int mtidx) {
		check_stack cs(s,-1);
        if (mtidx < 0) {
            mtidx = s.gettop()+mtidx+1;
        }
        if (!s.istable(mtidx)) {
            s.error("need metatable");
        }
        s.pushstring(name); // func, name
		auto t = s.rawget(mtidx); // func, field
		if (t == value_type::none || t == value_type::lnil) {
			s.pop(1); // func
            s.pushstring(name); // func, name
            s.createtable(0,0); // func, name, field
			s.pushvalue(-3); // func, name, field, func
			s.seti(-2,metaobject__getters); // func, name, field
			set_metaobject_field_metatable(s);
			s.rawset(mtidx); // func
            s.pop(1);
			return;
		} else if (t == value_type::userdata) {
			s.pushvalue(-2);
			s.seti(-2,metaobject__getters);
            s.pop(2);
		}
		s.pop(2);
        s.error("override field %s on %s",name,s.tostring(mtidx));
	}

    void metatable_set_method(state& s,const char* name, int mtidx) {
        check_stack cs(s,-1);
        if (mtidx < 0) {
            mtidx = s.gettop()+mtidx+1;
        }
        s.pushstring(name);
        s.pushvalue(-2);
        s.rawset(mtidx);
        s.pop(1);
    }

    void metatable_set_field(state& s,const char* name, int mtidx) {
        check_stack cs(s,-1);
        if (mtidx < 0) {
            mtidx = s.gettop()+mtidx+1;
        }
        s.pushstring(name);
        s.pushvalue(-2);
        s.rawset(mtidx);
        s.pop(1);
    }

	void ref_value(state& s,int idx,int ref_idx) {
		if (idx < 0) {
			idx = s.gettop()+idx+1;
		}
		if (!s.isuserdata(idx)) {
			s.error("invalid value type %s",s.get_typename(idx));
		}
		s.pushvalue(ref_idx);
		s.setuservalue(idx);
	}
}
