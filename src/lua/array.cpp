#include "array.h"

namespace lua {

    static int array_ref__index(lua_State* L) {
        state s(L);
        auto hdr = array_ref_holder_base_t::get(s,1);
		if (hdr) {
			auto idx = s.checkinteger(2) - 1;
			if (idx<0 || idx>=hdr->length) {
				s.pushnil();
				return 1;
			}
			hdr->push_element(s,idx);
            return 1;
		} else {
            s.error("invalid array_ref");
        }
        return 0;
    }

    static int array_ref__newindex(lua_State* L) {
        state s(L);
		auto hdr = array_ref_holder_base_t::get(s,1);
		if (hdr) {
			if (hdr->is_const()) {
				s.error("attempt to modify a const array");
				return 0;
			}
            auto idx = s.checkinteger(2) - 1;
			if (idx<0 || idx>=hdr->length) {
				s.error("index out of bounds");
				return 0;
			}
            hdr->set_element(s,idx,3);
		} else {
            s.error("invalid array_ref");
        }
		return 0;
    }

    static int array_ref__gc(lua_State* L) {
        state s(L);
		auto hdr = array_ref_holder_base_t::get(s,1);
		if (hdr) {
			hdr->~array_ref_holder_base_t();
		}
		return 0;
    }

    static int array_ref__len(lua_State* L) {
        state s(L);
		auto hdr = array_ref_holder_base_t::get(s,1);
		if (hdr) {
			s.pushinteger(hdr->length);
			return 1;
		}
		return 0;
    }

    static int array_ref__tostring(lua_State* L) {
        state s(L);
        if (auto hdr = array_ref_holder_base_t::get(s,1)) {
            s.pushfstring("array_ref: %s",hdr->info()->name);
        } else {
            s.pushstring("unknown");
        }
        return 1;
    }

    static void get_array_ref_metatable(state& s) {
		auto t = s.getmetatable("array_ref");
		if (t != value_type::table) {
			s.pop(1);
			s.newmetatable("array_ref");
            s.pushcclosure(&array_ref__index,0);
            s.setfield(-2,"__index");
            s.pushcclosure(&array_ref__newindex,0);
            s.setfield(-2,"__newindex");
            s.pushcclosure(&array_ref__len,0);
            s.setfield(-2,"__len");
            s.pushcclosure(&array_ref__gc,0);
            s.setfield(-2,"__gc");
            s.pushcclosure(&array_ref__tostring,0);
            s.setfield(-2,"__tostring");
		}
	}

    void set_array_ref_metatable(state& s,const meta::info_t* info) {
        get_array_ref_metatable(s);
        s.setmetatable(-2);
    }

}