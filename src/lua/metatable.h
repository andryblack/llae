#ifndef _LLAE_LUA_METATABLE_H_INCLUDED_
#define _LLAE_LUA_METATABLE_H_INCLUDED_

#include "state.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include <new>
#include <utility>

namespace lua {

    struct meta_holder_base_t {
        
        const uint32_t marker = 0;
        explicit meta_holder_base_t(uint32_t marker) : marker(marker) {}
        
        virtual ~meta_holder_base_t() {}
        virtual const meta::info_t* info() const = 0;
        virtual void* get_raw_ptr() const = 0;
        template <typename T>
        T* get_ptr() {
            if (meta::is_convertible(info(), meta::info<T>::get()))
                return static_cast<T*>(get_raw_ptr());
            return nullptr;
        }
        static meta_holder_base_t* get(state& s,int idx) {
            void* data = s.touserdata(idx);
            if (!data) return nullptr;
            if (s.rawlen(idx)<sizeof(meta_holder_base_t)) {
                return nullptr;
            }
            return static_cast<meta_holder_base_t*>(data);
        }
        static meta_holder_base_t* get(state& s,int idx,uint32_t type_marker,size_t size) {
            void* data = s.touserdata(idx);
            if (!data) return nullptr;
            if (s.rawlen(idx)<size) {
                return nullptr;
            }
            auto ret = static_cast<meta_holder_base_t*>(data);
            if (ret->marker != type_marker) {
                return nullptr;
            }
            return ret;
        }
        template <typename T>
        static T* get_ptr(state& l,int idx);
    };

    template <typename T>
    struct raw_holder_t : meta_holder_base_t {
        static constexpr uint32_t type_marker = 0xb1c14011;
        mutable T raw;
        ~raw_holder_t() {
            assert(marker == type_marker);
        }
        explicit raw_holder_t(T&& val) : meta_holder_base_t(type_marker), raw(std::move(val)) {}
        const meta::info_t* info() const override { return ::meta::info<T>::get(); }
        virtual void* get_raw_ptr() const override {
            return &raw;
        }
        static raw_holder_t* get(state& s,int idx) {
            return static_cast<raw_holder_t*>(meta_holder_base_t::get(s,idx,type_marker,sizeof(raw_holder_t)));
        }
    };

	struct object_holder_t : meta_holder_base_t {
        using hold_t = common::intrusive_ptr<meta::object>;
        hold_t hold;
        static constexpr uint32_t type_marker = 0xb1c1401d;
        
        explicit object_holder_t(const hold_t& hold) : meta_holder_base_t(type_marker), hold(hold) {}
        explicit object_holder_t( hold_t&& hold ) : meta_holder_base_t(type_marker), hold(std::move(hold)) {}
        
        ~object_holder_t() {
            assert(marker == type_marker);
            hold.reset();
        }

        const meta::info_t* info() const override { return hold ? hold->get_object_info() : nullptr; }
        virtual void* get_raw_ptr() const override { return hold.get(); }
		template <class T>
		common::intrusive_ptr<T> get_intrusive() const {
			return common::intrusive_ptr<T>(meta::cast<T>(hold.get()));
		}
		template <class T>
		T* get_raw() const {
			return meta::cast<T>(hold.get());
		}
		static object_holder_t* get(state& s,int idx) {
            auto ret = static_cast<object_holder_t*>(meta_holder_base_t::get(s,idx,type_marker,sizeof(object_holder_t)));
            if (!ret || !ret->hold)
                return nullptr;
            return ret;
		}
	};

    template <typename T>
    T* meta_holder_base_t::get_ptr(state& l,int idx) {
        auto holder = meta_holder_base_t::get(l,idx);
        if (!holder) return nullptr;
        return holder->get_ptr<T>();
    }

	void register_meta_object_metatable(state& s);
	void create_metatable(state& s,const meta::info_t* info);
	void set_metatable(state& s,const meta::info_t* info);
	void get_metatable(state& s,const meta::info_t* info);

	template <class T>
	static void push_meta_object( state& s,common::intrusive_ptr<T>&& v ) {
		void* data = s.newuserdata(sizeof(object_holder_t));
		const meta::info_t* info = v->get_object_info();
		new (data) object_holder_t{ std::move(v)  };
		set_metatable(s,info);
	}
    template <class T>
    static void push_meta_object( state& s,const common::intrusive_ptr<T>& v ) {
        void* data = s.newuserdata(sizeof(object_holder_t));
        const meta::info_t* info = v->get_object_info();
        new (data) object_holder_t{ v  };
        set_metatable(s,info);
    }
    template <class T>
    static void push_raw( state& s,T&& v ) {
        using holder_t = raw_holder_t<T>;
        void* data = s.newuserdata(sizeof(holder_t));
        const meta::info_t* info = meta::info<T>::get();
        new (data) holder_t{ std::move(v) };
        set_metatable(s,info);
    }

}

#endif /*_LLAE_LUA_METATABLE_H_INCLUDED_*/ 
