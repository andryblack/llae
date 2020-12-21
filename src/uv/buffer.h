#ifndef __LLAE_UV_BUFFER_H_INCLUDED__
#define __LLAE_UV_BUFFER_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "decl.h"
#include "meta/object.h"
#include "lua/ref.h"
#include "lua/state.h"
#include <vector>
#include <cstdlib>


namespace uv {

    class buffer;
    typedef common::intrusive_ptr<buffer> buffer_ptr;
    class buffer : public meta::object {
        META_OBJECT
    private:
        uv_buf_t m_buf;
        size_t m_capacity;
    protected:
        void destroy() override;
        struct buffer_alloc_tag {
            char* data;
            size_t size;
        };
        explicit buffer(const buffer_alloc_tag&);
    public:
        static buffer_ptr alloc(size_t size) {
            return alloc_obj<buffer>(size);
        }
        static buffer_ptr hold(const void* data,size_t size) {
            auto res = alloc(size);
            std::memcpy(res->get_base(),data,size);
            return res;
        }
        template <class Extend,typename...Args>
        static common::intrusive_ptr<Extend> alloc_obj(size_t size,Args...args) {
            void* mem = std::malloc(sizeof(Extend)+size);
            char* data = static_cast<char*>(mem) + sizeof(Extend);
            buffer_alloc_tag tag = {data,size};
            Extend* b = new (mem) Extend(tag,args...);
            return common::intrusive_ptr<Extend>(b);
        }
        size_t get_len() const { return m_buf.len; }
        size_t get_capacity() const { return m_capacity; }
        void* get_base() { return m_buf.base; }
        void* get_end() { return m_buf.base + m_buf.len;}
        void set_len(size_t l){m_buf.len=l;}
        lua::multiret sub(lua::state& l);
        const void* get_base() const { return m_buf.base; }
        static buffer* get(uv_buf_t* b);
        static buffer* get(char* base);
        uv_buf_t* get() { return &m_buf;}
        lua::multiret hex(lua::state& l);
        lua::multiret lfind(lua::state& l);
        lua::multiret lbyte(lua::state& l);
        static lua::multiret lconcat(lua::state& l);
        static void lbind(lua::state& l);
    };

    class write_buffers {
    private:
        std::vector<uv_buf_t> m_bufs;
        std::vector<lua::ref> m_refs;
        std::vector<buffer_ptr> m_ext;
        bool put_one(lua::state& s);
    public:
        bool put(lua::state& s);
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

