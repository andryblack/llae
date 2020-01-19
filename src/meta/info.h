#ifndef __LLAE_META_INFO_H_INCLUDED__
#define __LLAE_META_INFO_H_INCLUDED__

#include <cstddef>

namespace meta {

	struct info_t;
	struct parent_info_t {
		const info_t* info;
		void* (*downcast)(void *);
	};
	struct info_t {
		const char* name;
		std::size_t size;
		parent_info_t parent;
	};

    namespace impl {
    	template <class Child,class Parent>
	   	static void* downcast(void* ptr) {
	   		return static_cast<Parent*>(static_cast<Child*>(ptr));
	   	}
	}

    static inline bool is_convertible( const info_t* from, const info_t* to ) {
        do {
            if ( from == to ) return true;
            from = from->parent.info;
        } while (from && to);
        return false;
    }




}

#endif /*__LLAE_META_INFO_H_INCLUDED__*/