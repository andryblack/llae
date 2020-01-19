#ifndef __LLAE_META_OBJECT_H_INCLUDED__
#define __LLAE_META_OBJECT_H_INCLUDED__

#include "info.h"
#include <cassert>

namespace meta {

	class object {
        object(const object&) = delete;
        object& operator = (const object&) = delete;
    private:
        std::size_t m_refs;
        static const info_t m_meta_info_storage;
    protected:
        object() : m_refs(0) {}
        virtual ~object() { assert(m_refs==0); }
        void release() { delete this; }
    public:
        void add_ref() { ++m_refs; }
        void remove_ref() { assert(m_refs>0); --m_refs; if (m_refs==0) release(); }
        static const info_t* get_class_info() { return &m_meta_info_storage; }
        virtual const info_t* get_object_info() const { return get_class_info(); }
	};

#define META_OBJECT \
    public: \
        static const ::meta::info_t* get_class_info() { return &m_meta_info_storage; }\
        virtual const ::meta::info_t* get_object_info() const override { return get_class_info(); } \
    private: \
        static const ::meta::info_t m_meta_info_storage;

#define META_OBJECT_INFO_X(Klass,Parent,Name) \
        const ::meta::info_t Klass::m_meta_info_storage = { \
            Name, sizeof(Klass), { Parent::get_class_info(), &meta::impl::downcast<Klass,Parent> } \
        };

#define META_OBJECT_INFO(Type,Parent) META_OBJECT_INFO_X(Type,Parent,#Type)

    

}

#endif /*__LLAE_META_OBJECT_H_INCLUDED__*/