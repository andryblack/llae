#include "buffer.h"
#include "lua/bind.h"
#include <new>
#include <cstddef>
#include <cstdlib>
#include <memory>

META_OBJECT_INFO(uv::buffer,meta::object)

namespace uv {

    static const char hex_char[] = "0123456789abcdef";

    buffer::buffer(const buffer_alloc_tag& tag) : m_capacity(tag.size) {
        m_buf.base = tag.data;
        m_buf.len = tag.size;
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

    lua::multiret buffer::sub(lua::state& l) {
        if (get_len()==0) {
            l.pushstring("");
            return {1};
        }
        lua_Integer begin = l.optinteger(2,1)-1;
        lua_Integer end = l.optinteger(3,get_len())-1;
        if (begin<0){
            begin = 0;
        }
        if (begin >= get_len()) {
            begin = get_len()-1;
        }
        if (end < 0) {
            end = 0;
        }
        if (end >= get_len()) {
            end = get_len()-1;
        }
        if (begin>=end) {
            l.pushstring("");
            return {1};
        }
        l.pushlstring(m_buf.base+begin,end-begin+1);
        return {1};
    }

    lua::multiret buffer::hex(lua::state& l) {
        size_t size = get_len()*2;
        std::unique_ptr<char[]> buf(new char[size]);
        char* dst = buf.get();
        const unsigned char* d = static_cast<const unsigned char*>(get_base());
        for (size_t i=0;i<get_len();++i) {
            *dst++ = hex_char[(*d>>4) & 0xf];
            *dst++ = hex_char[*d & 0xf];
            ++d;
        }
        l.pushlstring(buf.get(),size);
        return {1};
    }

    void buffer::lbind(lua::state& l) {
        lua::bind::function(l,"get_len",&buffer::get_len);
        lua::bind::function(l,"__len",&buffer::get_len);
        lua::bind::function(l,"sub",&buffer::sub);
        lua::bind::function(l,"hex",&buffer::hex);
    }

    bool write_buffers::put_one(lua::state& l) {
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
                return false;
            }
        }
        return true;
    }
    bool write_buffers::put(lua::state &l) {
        auto t = l.get_type(-1);
        if (t == lua::value_type::table) {
            size_t tl = l.rawlen(-1);
            m_bufs.reserve(tl);
            m_refs.reserve(tl);
            for (size_t j=0;j<tl;++j) {
                l.rawgeti(-1,j+1);
                if (!put_one(l)) {
                    l.pop(2);
                    return false;
                }
            }
            l.pop(1);
        } else {
            if (!put_one(l)) {
                l.pop(1);
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

    void write_buffers::release() {
        for (auto& r:m_refs) {
            r.release();
        }
    }

    void write_buffers::pop_front(lua::state& l) {
        m_refs.front().reset(l);
        m_refs.erase(m_refs.begin());
        m_bufs.erase(m_bufs.begin());
    }

}
