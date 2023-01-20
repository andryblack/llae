#ifndef __LLAE_UV_BUFFER_H_INCLUDED__
#define __LLAE_UV_BUFFER_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "decl.h"
#include "meta/object.h"
#include "lua/ref.h"
#include "lua/state.h"
#include "llae/memory.h"
#include <vector>
#include <cstdlib>


namespace uv {
    
    class buffer_base;
    typedef common::intrusive_ptr<buffer_base> buffer_base_ptr;
    class buffer;
    typedef common::intrusive_ptr<buffer> buffer_ptr;

    class buffer_base : public meta::object {
        META_OBJECT
        LLAE_NAMED_ALLOC(buffer_base)
    protected:
        uv_buf_t    m_buf;
        buffer_base() {}
        explicit buffer_base(const uv_buf_t& b) : m_buf(b) {}
    public:
        const uv_buf_t* get() const { return &m_buf;}
        size_t get_len() const { return m_buf.len; }
        lua::multiret sub(lua::state& l) const;
        buffer_ptr reverse() const;
        const void* get_base() const { return m_buf.base; }
        lua::multiret lfind(lua::state& l) const;
        lua::multiret lbyte(lua::state& l) const;
        lua::multiret ltostring(lua::state& l) const;
        lua::multiret leq(lua::state& l) const;
        
        static void lbind(lua::state& l);
        static buffer_base_ptr get(lua::state& l,int idx,bool check=false);
        static lua::multiret lconcat(lua::state& l);

        static lua::multiret hex_decode(lua::state& l);
        static lua::multiret hex_encode(lua::state& l);
        static lua::multiret base64_decode(lua::state& l);
        static lua::multiret base64_encode(lua::state& l);
    };

    
    class buffer : public buffer_base {
        META_OBJECT
        LLAE_NAMED_ALLOC(buffer_base)
    private:
        const size_t m_capacity;
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
            void* mem = allocator_t::alloc(sizeof(Extend)+size);
            char* data = static_cast<char*>(mem) + sizeof(Extend);
            buffer_alloc_tag tag = {data,size};
            Extend* b = new (mem) Extend(tag,args...);
            return common::intrusive_ptr<Extend>(b);
        }
        
        size_t get_capacity() const { return m_capacity; }
        void* get_base() { return m_buf.base; }
        void* get_end() { return m_buf.base + m_buf.len;}
        void set_len(size_t l){m_buf.len=l;}
        void self_reverse();
        
        static buffer* get(uv_buf_t* b);
        static buffer* get(char* base);
        uv_buf_t* get() { return &m_buf;}
       
        
        void* find(const char* str);
        buffer_ptr realloc(size_t len);
       
        static lua::multiret lnew(lua::state& l);
        static lua::multiret lalloc(lua::state& l);
        static void lbind(lua::state& l);
        static buffer_ptr get(lua::state& l,int idx,bool check=false);
    };

    class write_buffers {
    private:
        std::vector<uv_buf_t> m_bufs;
        std::vector<lua::ref> m_refs;
        std::vector<buffer_base_ptr> m_ext;
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

