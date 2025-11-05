#pragma once

#include "state.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include <new>
#include <utility>
#include <type_traits>

namespace lua {

    struct meta_holder_base_t {
        
        const uint32_t marker = 0;
        explicit meta_holder_base_t(uint32_t marker) : marker(marker) {}
        
        virtual ~meta_holder_base_t() {}
        virtual const meta::info_t* info() const = 0;
        virtual void* get_raw_ptr() const = 0;
        virtual bool is_const() const = 0;
        template <typename T>
        T* get_ptr(bool& is_const_invalid) {
            if (meta::is_convertible(info(), meta::info<T>::get())) {
                if (!std::is_const<T>::value && is_const()) {
                    is_const_invalid = true;
                    return nullptr;
                }
                return static_cast<T*>(get_raw_ptr());
            }
            return nullptr;
        }
        template <typename T>
        T* get_ptr() {
            if (meta::is_convertible(info(), meta::info<T>::get())) {
                if (!std::is_const<T>::value && is_const()) {
                    return nullptr;
                }
                return static_cast<T*>(get_raw_ptr());
            }
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
        template <typename T>
        static T* get_ptr(state& l,int idx,bool& is_const_invalid);
    };

    template <typename T>
    struct raw_holder_t : meta_holder_base_t {
        struct inplace {};
        static constexpr uint32_t type_marker = 0xb1c14011;
        mutable T raw;
        virtual ~raw_holder_t() override {
            assert(marker == type_marker);
        }
        virtual bool is_const() const override { return std::is_const<T>::value; }
        explicit raw_holder_t(T&& val) : meta_holder_base_t(type_marker), raw(std::move(val)) {}
        template <typename ... Args>
        explicit raw_holder_t(inplace,Args&&... args) : meta_holder_base_t(type_marker), raw(std::forward<Args>(args)...) {}
        const meta::info_t* info() const override { return ::meta::info<T>::get(); }
        virtual void* get_raw_ptr() const override {
            return &raw;
        }
        static raw_holder_t* get(state& s,int idx) {
            return static_cast<raw_holder_t*>(meta_holder_base_t::get(s,idx,type_marker,sizeof(raw_holder_t)));
        }
    };

    template <typename T>
    struct ptr_holder_t : meta_holder_base_t {
        static constexpr uint32_t type_marker = 0xb2c24022;
        mutable T* ptr = nullptr;
        virtual ~ptr_holder_t() override {
            assert(marker == type_marker);
        }
        explicit ptr_holder_t(T* val) : meta_holder_base_t(type_marker), ptr(val) {}
        const meta::info_t* info() const override { return ::meta::info<T>::get(); }
        virtual void* get_raw_ptr() const override { return const_cast<typename std::remove_const<T>::type*>(ptr); }
        virtual bool is_const() const override { return std::is_const<T>::value; }
        static ptr_holder_t* get(state& s,int idx) {
            return static_cast<ptr_holder_t*>(meta_holder_base_t::get(s,idx,type_marker,sizeof(ptr_holder_t)));
        }
    };

    struct object_holder_base_t : meta_holder_base_t {
        virtual void release() = 0;
        static constexpr uint32_t type_marker = 0xb1c1401d;
        object_holder_base_t() : meta_holder_base_t(type_marker) {}
        virtual ~object_holder_base_t() override {
            assert(marker == type_marker);
        }
        static object_holder_base_t* get(state& s,int idx) {
            auto ret = static_cast<object_holder_base_t*>(meta_holder_base_t::get(s,idx,type_marker,sizeof(object_holder_base_t)));
            return ret;
		}
    };
	template <typename T>
	struct object_holder_t : object_holder_base_t {
        using hold_t = common::intrusive_ptr<T>;
        hold_t hold;
        
        
        explicit object_holder_t(const hold_t& hold) : object_holder_base_t(), hold(hold) {}
        explicit object_holder_t( hold_t&& hold ) : object_holder_base_t(), hold(std::move(hold)) {}
        
        virtual ~object_holder_t() override {
            hold.reset();
        }

        const meta::info_t* info() const override { return hold ? hold->get_object_info() : nullptr; }
        virtual void* get_raw_ptr() const override { return hold.get(); }
        virtual bool is_const() const override { return std::is_const<T>::value; }
        virtual void release() override { hold.reset(); }
        template <class U>
		common::intrusive_ptr<U> get_intrusive() const {
			return common::intrusive_ptr<U>(meta::cast<U>(hold.get()));
		}
		static object_holder_t* get(state& s,int idx) {
            auto ret = static_cast<object_holder_t<T>*>(meta_holder_base_t::get(s,idx,type_marker,sizeof(object_holder_t<T>)));
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

    template <typename T>
    T* meta_holder_base_t::get_ptr(state& l,int idx,bool& is_const_invalid ) {
        auto holder = meta_holder_base_t::get(l,idx);
        if (!holder) return nullptr;
        return holder->get_ptr<T>(is_const_invalid);
    }

	void register_meta_object_metatable(state& s);
	void create_metatable(state& s,const meta::info_t* info);
	void set_metatable(state& s,const meta::info_t* info);
	void get_metatable(state& s,const meta::info_t* info);
    void metatable_set_setter(state& s, const char* name, int mtidx);
    void metatable_set_getter(state& s, const char* name, int mtidx);
    void metatable_set_method(state& s,const char* name, int mtidx);
    void metatable_set_field(state& s,const char* name, int mtidx);

    void ref_value(state& s,int idx,int ref_idx);

	template <class T>
	static void push_meta_object( state& s,common::intrusive_ptr<T>&& v ) {
		void* data = s.newuserdata(sizeof(object_holder_t<T>));
		const meta::info_t* info = v->get_object_info();
		new (data) object_holder_t<T>{ std::move(v)  };
		set_metatable(s,info);
	}
    template <class T>
    static void push_meta_object( state& s,const common::intrusive_ptr<T>& v ) {
        void* data = s.newuserdata(sizeof(object_holder_t<T>));
        const meta::info_t* info = v->get_object_info();
        new (data) object_holder_t<T>{ v  };
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

    template <class T,typename ... Args>
    static void construct_raw( state& s,Args&&... args ) {
        using holder_t = raw_holder_t<T>;
        void* data = s.newuserdata(sizeof(holder_t));
        const meta::info_t* info = meta::info<T>::get();
        using inplace_t = typename holder_t::inplace;
        new (data) holder_t{ inplace_t{}, std::forward<Args>(args)... };
        set_metatable(s,info);
    }

    template <class T>
    static void push_ptr( state& s,T* v ) {
        using holder_t = ptr_holder_t<T>;
        void* data = s.newuserdata(sizeof(holder_t));
        const meta::info_t* info = meta::info<T>::get();
        new (data) holder_t{ v };
        set_metatable(s,info);
    }

}
