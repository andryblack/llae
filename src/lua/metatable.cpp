#include "metatable.h"

namespace lua {

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