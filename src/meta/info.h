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
    template <typename T,class enable = void>
    struct info {
        static const info_t* get();
    };
    template <>
    struct info<void,void> {
        static const info_t* get() { return nullptr; }
    };
    template <typename T>
    struct info<const T> : info<T> {};

#define META_INFO_BASE_X(Klass,Name,Base) \
        template <> \
        const ::meta::info_t* ::meta::info<Klass>::get() { \
            static const ::meta::info_t info = { Name,  ::meta::info<Base>::get() }; \
            return &info; \
        };

#define META_INFO(Type,Base) META_INFO_BASE_X(Type,#Type,Base)


}

#endif /*__LLAE_META_INFO_H_INCLUDED__*/
