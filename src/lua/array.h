#pragma once

#include "metatable.h"
#include "stack.h"

namespace lua {

    struct array_ref_holder_base_t : meta_holder_base_t {
        static constexpr uint32_t type_marker = 0xa11a7401;
        mutable void* ptr = nullptr;
        mutable size_t length = 0;
        array_ref_holder_base_t(void* ptr,size_t length) : meta_holder_base_t(type_marker), ptr(ptr), length(length) {}
        virtual ~array_ref_holder_base_t() override {
            assert(marker == type_marker);
        }
        virtual void* get_raw_ptr() const override { return ptr; }
        static array_ref_holder_base_t* get(state& s,int idx) {
            return static_cast<array_ref_holder_base_t*>(meta_holder_base_t::get(s,idx,type_marker,sizeof(array_ref_holder_base_t)));
        }
        virtual void* get_element(size_t idx) const = 0;
        virtual void push_element(state& s,size_t idx) const = 0;
        virtual void set_element(state& s,size_t idx,int value_idx) = 0;
    };
    template <typename T, typename P>
    struct array_ref_holder_t : array_ref_holder_base_t {
        using policy_t = P;
        using element_type = typename std::remove_const<T>::type;
        virtual ~array_ref_holder_t() override {
            assert(marker == type_marker);
        }
        explicit array_ref_holder_t(T* val,size_t length) : array_ref_holder_base_t(const_cast<element_type*>(val),length) {}
        const meta::info_t* info() const override { return ::meta::info<T>::get(); }
        virtual bool is_const() const override { return std::is_const<T>::value; }
        static array_ref_holder_t* get(state& s,int idx) {
            return static_cast<array_ref_holder_t*>(array_ref_holder_base_t::get(s,idx));
        }
        element_type& get_element_ref(size_t idx) const {
            return static_cast<element_type*>(ptr)[idx];
        }
        virtual void* get_element(size_t idx) const override {
            return const_cast<element_type*>(static_cast<const T*>(ptr)) + idx;
        }
        virtual void push_element(state& s,size_t idx) const override {
            policy_t::template push_field<T>(s,get_element_ref(idx));
        }
        virtual void set_element(state& s,size_t idx,int value_idx) override {
            get_element_ref(idx) = policy_t::template get_field<T>(s,value_idx);
        }
    };

    void set_array_ref_metatable(state& s,const meta::info_t* info);

    template <typename P, typename T>
    void push_array_ref(state& s,T* val,size_t length,int parent_idx) {
        using holder_t = array_ref_holder_t<T,P>;
        void* data = s.newuserdata(sizeof(holder_t));
        new (data) holder_t( val, length );
        set_array_ref_metatable(s,::meta::info<T>::get());
        ref_value(s,-1,parent_idx);
    }

}