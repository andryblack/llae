#include "archive.h"
#include <zlib.h>

namespace archive {
	bool inflate(const void* src,size_t src_size,void* dst,size_t& dst_size) {
	    uLongf dstLen = dst_size;
	    int r = uncompress(static_cast<Bytef*>(dst),&dstLen,static_cast<const Bytef*>(src),src_size);
	    if (r!=Z_OK) {
	        dst_size = dstLen;
	        return false;
	    }
	    return true;
	}
    bool deflate(const void* src,size_t src_size,void* dst,size_t& dst_size) {
        uLongf dstLen = dst_size;
        int r = compress(static_cast<Bytef*>(dst),&dstLen,static_cast<const Bytef*>(src),src_size);
        if (r!=Z_OK) {
            dst_size = dstLen;
            return false;
        }
        return true;
    }
}
