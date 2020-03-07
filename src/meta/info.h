#ifndef __LLAE_META_INFO_H_INCLUDED__
#define __LLAE_META_INFO_H_INCLUDED__

#include <cstddef>

namespace meta {

	struct info_t;
	struct info_t {
		const char* name;
		const info_t* parent;
	};

    static inline bool is_convertible( const info_t* from, const info_t* to ) {
        while (from && to) {
            if ( from == to ) return true;
            from = from->parent;
        }
        return false;
    }




}

#endif /*__LLAE_META_INFO_H_INCLUDED__*/