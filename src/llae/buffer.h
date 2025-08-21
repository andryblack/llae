#pragma once

#include "common/intrusive_ptr.h"
#include "meta/object.h"
#include "memory.h"
#include "lua/state.h"
#include <cstdint>

namespace llae {

    class buffer_base;
    typedef common::intrusive_ptr<buffer_base> buffer_base_ptr;
    class buffer;
    typedef common::intrusive_ptr<buffer> buffer_ptr;

    class buffer_view {
    protected:
        const void* m_data;
        const size_t m_size;
    public:
        buffer_view() : m_data(nullptr),m_size(0) {}
        buffer_view(const void* data,size_t len) : m_data(data),m_size(len) {}

        const void* get_base() const { return m_data; }
        size_t get_len() const { return m_size; }
        bool empty() const { return m_size == 0; }

        static buffer_view get(lua::state& l,int idx,bool check=false);
    };

    class buffer_base : public meta::object {
        META_OBJECT
        LLAE_NAMED_ALLOC(buffer_base)
    protected:
        buffer_base() {}
        buffer_base(void* data,size_t size) : m_data(data),m_size(size) {}
    public:
        operator buffer_view() const { return buffer_view(m_data,m_size); }

        const void* get_base() const { return m_data; }
        size_t get_len() const { return m_size; }

        lua::multiret sub(lua::state& l) const;
        buffer_ptr reverse() const;
        
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

    protected:
        void* m_data = nullptr;
        size_t m_size = 0;
    };

    class buffer : public buffer_base {
        META_OBJECT
        LLAE_NAMED_ALLOC(buffer_base)
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
        void* get_base() { return m_data; }
        void* get_end() { return static_cast<uint8_t*>(m_data) + m_size;}
        void set_len(size_t l){m_size=l;}
        void self_reverse();
        
        void* find(const char* str);
        buffer_ptr realloc(size_t len);
       
        static lua::multiret lnew(lua::state& l);
        static lua::multiret lalloc(lua::state& l);
        static void lbind(lua::state& l);
      
        static buffer_ptr get(lua::state& l,int idx,bool check = false);
        static buffer* get( char*);
    private:
        const size_t m_capacity;
    };
}

#include "lua/stack.h"

namespace lua {
    template<>
    struct stack<llae::buffer_view> {
        static llae::buffer_view get(lua::state& l,int idx) {
            return llae::buffer_view::get(l,idx,false);
        }
    };
    template<>
    struct stack<const llae::buffer_view&> : stack<llae::buffer_view> {};
    template<>
    struct stack<check<llae::buffer_view> > {
        static llae::buffer_view get(lua::state& l,int idx) {
            return llae::buffer_view::get(l,idx,true);
        }
    };
}