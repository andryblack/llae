#pragma once

#include "llae/buffer.h"
#include "lua/ref.h"
#include "lua/stack.h"
#include <vector>
#include <cstdlib>


namespace llae {
    

    class write_buffers {
    private:
        std::vector<buffer_view> m_bufs;
        std::vector<lua::ref> m_refs;
        std::vector<buffer_base_ptr> m_ext;
        bool put_one(lua::state& s);
    public:
        bool put(lua::state& s);
        bool putm(lua::state& s,int base);
        void reset(lua::state& l);
        void release();
        const std::vector<buffer_view>& get_buffers() const { return m_bufs; }
        bool empty() const { return m_bufs.empty(); }
        void pop_front(lua::state& l);
        size_t get_total_size() const {
            size_t res = 0;
            for (auto& b:m_bufs) {
                res += b.get_len();
            }
            return res;
        }
    };
    
}


