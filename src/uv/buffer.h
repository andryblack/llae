#ifndef __LLAE_UV_BUFFER_H_INCLUDED__
#define __LLAE_UV_BUFFER_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "decl.h"
#include "meta/object.h"
#include "lua/ref.h"
#include "lua/state.h"
#include <vector>


namespace uv {

    class buffer;
    typedef common::intrusive_ptr<buffer> buffer_ptr;
    class buffer : public meta::object {
        META_OBJECT
    private:
        uv_buf_t m_buf;
        buffer(char* data,size_t size);
        void destroy() override final;
    public:
        static buffer_ptr alloc(size_t size);
        size_t get_len() const { return m_buf.len; }
        void* get_base() { return m_buf.base; }
        const void* get_base() const { return m_buf.base; }
        static buffer* get(uv_buf_t* b);
        static buffer* get(char* base);
    };

    class write_buffers {
    private:
        std::vector<uv_buf_t> m_bufs;
        std::vector<lua::ref> m_refs;
    public:
        void put(lua::state& s);
        void reset(lua::state& l);
        const std::vector<uv_buf_t>& get_buffers() const { return m_bufs; }
        bool empty() const { return m_bufs.empty(); }
        void pop_front(lua::state& l);
    };
    
}

#endif /*__LLAE_UV_STREAM_H_INCLUDED__*/

