#ifndef __LLAE_MEMORY_H_INCLUDED__
#define __LLAE_MEMORY_H_INCLUDED__

#include <new>
#include <atomic>

namespace llae {
	template <class tag>
	class named_alloc {
	private:
		static inline std::atomic<size_t> m_allocated = 0;
	public:
        static size_t get_allocated() { return m_allocated; }
		static void* alloc(size_t size) {
			m_allocated += size;
			return ::operator new(size);
		}
		static void dealloc(void* ptr,size_t size) {
			if (m_allocated < size) {
				report_diag_error("too many dealloc",__FILE__,__LINE__);
			}
			m_allocated -= size;
			::operator delete(ptr,size);
		}
	};
#define LLAE_NAMED_ALLOC(tag) \
	using allocator_t = ::llae::named_alloc<tag>; \
	static void* operator new(size_t size) { \
		return allocator_t::alloc(size); \
	} \
	static void* operator new(size_t size,void* at) { \
		return ::operator new (size,at); \
	} \
	static void operator delete(void* ptr,size_t size) { \
		return allocator_t::dealloc(ptr,size); \
	}
}

#endif /*__LLAE_MEMORY_H_INCLUDED__*/
