#include "metatable.h"
#include "stack.h"

namespace lua {

	static int metaobject_destroy(lua_State* L) {
		auto hdr = static_cast<object_holder_t*>(state(L).touserdata(1));
		if (hdr) {
			hdr->~object_holder_t();
		}
		return 0;
	}

	void create_metatable(state& s,const meta::info_t* info) {
		if (!s.newmetatable(info->name)) {
			s.pop(1);
			s.error("metatable %s already registered",info->name);
		}
		const meta::info_t* parent = info->parent;
		if (parent) {
			if (s.getmetatable(parent->name) == value_type::nil ) {
				s.error("unregistered parent metatable %s",parent->name);
			}
			s.setmetatable(-2);
		}
		s.pushvalue(-1);
		s.setfield(-2,"__index");
		s.pushcclosure(&metaobject_destroy,0);
		s.setfield(-2,"__gc");
		s.pushinteger(object_holder_t::type_marker);
		s.setfield(-2,"__llae_type");
	}

	void set_metatable(state& s,const meta::info_t* info) {
		if (s.getmetatable(info->name) == value_type::nil) {
			s.error("not registered metatable %s",info->name);
		}
		s.setmetatable(-2);
	}
	void get_metatable(state& s,const meta::info_t* info) {
		s.getmetatable(info->name);
	}

}