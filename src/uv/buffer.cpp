#include "buffer.h"
#include "lua/bind.h"
#include <new>
#include <cstddef>
#include <cstdlib>

META_OBJECT_INFO(uv::buffer,meta::object)

namespace uv {

    buffer::buffer(char* data,size_t size) {
        m_buf.base = data;
        m_buf.len = size;
    }

    buffer_ptr buffer::alloc(size_t size) {
        void* mem = std::malloc(sizeof(buffer)+size);
        char* data = static_cast<char*>(mem) + sizeof(buffer);
        buffer* b = new (mem) buffer(data,size);
        return buffer_ptr(b);
    }

    void buffer::destroy() {
        void* mem = this;
        this->~buffer();
        std::free(mem);
    }

    buffer* buffer::get(uv_buf_t* b) {
        if (!b) return nullptr;
        // offsetof got warning
        static const size_t offset = static_cast<char*>(static_cast<void*>(&(static_cast<buffer*>(nullptr))->m_buf))-
            static_cast<char*>(static_cast<void*>(static_cast<buffer*>(nullptr)));
        void* start = reinterpret_cast<char*>(b) - offset;
        //void* start = reinterpret_cast<char*>(b)-offsetof(buffer,m_buf);
        return static_cast<buffer*>(start);
    }

    buffer* buffer::get(char* base) {
        if (!base) return nullptr;
        void* start = base - sizeof(buffer);
        return static_cast<buffer*>(start);
    }

    void buffer::lbind(lua::state& l) {
        lua::bind::function(l,"get_len",&buffer::get_len);
    }

    bool write_buffers::put(lua::state &l) {
        auto t = l.get_type(-1);
        if (t == lua::value_type::table) {
            size_t tl = l.rawlen(-1);
            m_bufs.reserve(tl);
            m_refs.reserve(tl);
            for (size_t j=0;j<tl;++j) {
                l.rawgeti(-1,j+1);
                auto buf = lua::stack<uv::buffer_ptr>::get(l, -1);
                if (buf) {
                    m_refs.emplace_back();
                    m_refs.back().set(l);
                    m_bufs.push_back(*buf->get());
                } else {
                    size_t size;
                    
                    const char* val = l.tolstring(-1,size);
                    if (val && size !=0) {
                        m_refs.emplace_back();
                        m_refs.back().set(l);
                        m_bufs.push_back(uv_buf_init(const_cast<char*>(val),size));
                    } else {
                        l.pop(2);
                        return false;
                    }
                }
            }
            l.pop(1);
        } else {
            size_t size;
            const char* val = l.tolstring(-1,size);
            if (val && size != 0) {
                m_refs.emplace_back();
                m_refs.back().set(l);
                m_bufs.push_back(uv_buf_init(const_cast<char*>(val),size));
            } else {
                return false;
            }
        }
        return true;
    }

    void write_buffers::reset(lua::state &l) {
        for (auto& r:m_refs) {
            r.reset(l);
        }
        m_bufs.clear();
        m_refs.clear();
    }

    void write_buffers::pop_front(lua::state& l) {
        m_refs.front().reset(l);
        m_refs.erase(m_refs.begin());
        m_bufs.erase(m_bufs.begin());
    }

}
