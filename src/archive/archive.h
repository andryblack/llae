#ifndef __LLAE_ARCHIVE_ARCHIVE_H_INCLUDED__
#define __LLAE_ARCHIVE_ARCHIVE_H_INCLUDED__

#include <cstring>

namespace archive {

	bool inflate(const void* src,size_t src_size,void* dst,size_t& dst_size);
		
}

#endif /*__LLAE_ARCHIVE_ARCHIVE_H_INCLUDED__*/