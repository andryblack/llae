#ifndef __LLAE_UV_BUFFER_H_INCLUDED__
#define __LLAE_UV_BUFFER_H_INCLUDED__

#include "llae/buffer.h"
#include "lua/ref.h"
#include "lua/stack.h"
#include <vector>
#include <cstdlib>
#include "luv.h"


namespace uv {
    

    class write_buffers {
    private:
        std::vector<uv_buf_t> m_bufs;
        std::vector<lua::ref> m_refs;
        std::vector<llae::buffer_base_ptr> m_ext;
        bool put_one(lua::state& s);
    public:
        bool put(lua::state& s);
        bool putm(lua::state& s,int base);
        void reset(lua::state& l);
        void release();
        const std::vector<uv_buf_t>& get_buffers() const { return m_bufs; }
        bool empty() const { return m_bufs.empty(); }
        void pop_front(lua::state& l);
        size_t get_total_size() const {
            size_t res = 0;
            for (auto& b:m_bufs) {
                res += b.len;
            }
            return res;
        }
    };
    
}


#endif /*__LLAE_UV_STREAM_H_INCLUDED__*/

