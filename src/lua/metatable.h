#ifndef _LLAE_LUA_METATABLE_H_INCLUDED_
#define _LLAE_LUA_METATABLE_H_INCLUDED_

#include "state.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include <new>

namespace lua {

	struct object_holder_t {
		static constexpr uint32_t type_marker = 0xb1c1401d;
		common::intrusive_ptr<meta::object> hold;
        ~object_holder_t() {
            hold.reset();
        }

		const meta::info_t* info() const { return hold->get_object_info(); }
		template <class T>
		common::intrusive_ptr<T> get_intrusive() const {
			return common::intrusive_ptr<T>(meta::cast<T>(hold.get()));
		}
		template <class T>
		T* get_raw() const {
			return meta::cast<T>(hold.get());
		}

		static object_holder_t* get(state& s,int idx) {
			void* data = s.touserdata(idx);
			if (!data) return nullptr;
			if (s.getmetafield(idx,"__llae_type") != lua::value_type::number) {
				s.pop(1);
				return nullptr;
			}
			auto v = s.tointeger(-1);
			s.pop(1);
			if (v != type_marker) {
				return nullptr;
			}
			auto ret = static_cast<object_holder_t*>(data);
            if (!ret->hold)
                return nullptr;
            return ret;
		}
	};


	void create_metatable(state& s,const meta::info_t* info);
	void set_metatable(state& s,const meta::info_t* info);
	void get_metatable(state& s,const meta::info_t* info);

	template <class T>
	static void push_meta_object( state& s,common::intrusive_ptr<T>&& v ) {
		void* data = s.newuserdata(sizeof(object_holder_t));
		const meta::info_t* info = v->get_object_info();
		new (data) object_holder_t{ std::move(v) };
		set_metatable(s,info);
	}
    template <class T>
    static void push_meta_object( state& s,const common::intrusive_ptr<T>& v ) {
        void* data = s.newuserdata(sizeof(object_holder_t));
        const meta::info_t* info = v->get_object_info();
        new (data) object_holder_t{ v };
        set_metatable(s,info);
    }


}

#endif /*_LLAE_LUA_METATABLE_H_INCLUDED_*/ 
