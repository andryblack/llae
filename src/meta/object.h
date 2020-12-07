#ifndef __LLAE_META_OBJECT_H_INCLUDED__
#define __LLAE_META_OBJECT_H_INCLUDED__

#include "info.h"
#include "llae/diag.h"
#include "common/ref_counter.h"
#include <cassert>

namespace meta {

	class object : public common::ref_counter_base {
    private:
        LLAE_DIAG(static size_t m_count;)
        LLAE_DIAG(size_t m_object_mark;)
    protected:
        object() {
            LLAE_DIAG(m_object_mark=0x900dfeed;)
            LLAE_DIAG(++m_count;)
        }
        virtual ~object() {
            LLAE_CHECK(m_object_mark==0x900dfeed)
            LLAE_DIAG(m_object_mark=0xfeedbad;)
            LLAE_DIAG(--m_count;)
        }
    public:
        LLAE_DIAG(static size_t get_total_count() { return m_count; })
        static const info_t* get_class_info();
        virtual const info_t* get_object_info() const { return get_class_info(); }
  	};

#define META_OBJECT \
    public: \
        static const ::meta::info_t* get_class_info();\
        virtual const ::meta::info_t* get_object_info() const override { return get_class_info(); } \

#define META_OBJECT_INFO_X(Klass,Parent,Name) \
        const ::meta::info_t* Klass::get_class_info() { \
            static const ::meta::info_t info = { Name,  Parent::get_class_info() }; \
            return &info; \
        };

#define META_OBJECT_INFO(Type,Parent) META_OBJECT_INFO_X(Type,Parent,#Type)

    template <class T>
    static T* cast( object* o ) {
        if (!o) return nullptr;
        if (!is_convertible(o->get_object_info(),T::get_class_info())) return nullptr;
        return static_cast<T*>(o);
    }

}

#endif /*__LLAE_META_OBJECT_H_INCLUDED__*/