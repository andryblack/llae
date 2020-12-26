#include "buffer.h"
#include "lua/bind.h"
#include "lua/stack.h"
#include <new>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <mbedtls/base64.h>

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

    lua::multiret buffer::ltostring(lua::state& l) {
        l.pushlstring(m_buf.base,get_len());
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

    lua::multiret buffer::base64_encode(lua::state& l) {
        size_t osize = 0;
        const unsigned char* src = nullptr;
        size_t src_size = 0;
        if (l.isstring(1)) {
            src = reinterpret_cast<const unsigned char*>(l.tolstring(1,src_size));
        } else {
            auto self = lua::stack<buffer_ptr>::get(l, 1);
            if (!self) l.argerror(1, "need buffer or string");
            src = static_cast<const unsigned char*>(self->get_base());
            src_size = self->get_len();
        }

        if (MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL!=mbedtls_base64_encode(nullptr,0,&osize,src,src_size)) {
            return {0};
        }
        buffer_ptr buf(buffer::alloc(osize));
        if (mbedtls_base64_encode(static_cast<unsigned char*>(buf->get_base()),osize,&osize,src,src_size)!=0) {
            return {0};
        }
        buf->set_len(osize);
        lua::push(l,buf);
        return {1};
    }

    lua::multiret buffer::base64_decode(lua::state& l) {
        size_t osize = 0;
        const unsigned char* src = nullptr;
        size_t src_size = 0;
        if (l.isstring(1)) {
            src = reinterpret_cast<const unsigned char*>(l.tolstring(1,src_size));
        } else {
            auto self = lua::stack<buffer_ptr>::get(l, 1);
            if (!self) l.argerror(1, "need buffer or string");
            src = static_cast<const unsigned char*>(self->get_base());
            src_size = self->get_len();
        }

        if (MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL!=mbedtls_base64_decode(nullptr,0,&osize,src,src_size)) {
            return {0};
        }
        buffer_ptr buf(buffer::alloc(osize));
        if (mbedtls_base64_decode(static_cast<unsigned char*>(buf->get_base()),osize,&osize,src,src_size)!=0) {
            return {0};
        }
        buf->set_len(osize);
        lua::push(l,buf);
        return {1};
    }

    lua::multiret buffer::lfind(lua::state& l) {
        size_t len = 0;
        const char* str = l.checklstring(2,len);
        int offset = l.optinteger(3,1) - 1;
        if (offset<1) {
            // @todo
            offset = 1;
        } 
        if (offset > get_len()) {
            offset = get_len();
        }
        if (len == 0) { 
            l.pushinteger(offset);
            return {1}; 
        }
        if (len > get_len()) { 
            return {0}; 
        }
        auto start = static_cast<const char*>(get_base()) + offset - 1;
        while (true) {
            size_t flen = ((static_cast<const char*>(get_base()) + get_len()) - start)-len + 1;
            const char* pos = static_cast<const char*>(memchr(start,*str,len));
            if (!pos) {
                return {0}; 
            }
            if (len==1 || (memcmp(pos,str,len)==0)) {
                l.pushinteger((pos-static_cast<const char*>(get_base()))+1);
                return {1}; 
            }
            start = pos + 1;
        }
    }

    lua::multiret buffer::lbyte(lua::state& l) {
        size_t len = 0;
        int start = l.optinteger(2,1);
        int end = l.optinteger(3,start);
        if (start<0) {
            return {0};
        }
        if (get_len()==0) {
            return {0};
        }
        if (end>get_len()) {
            end=get_len();
        }
        if (end<start) {
            return {0}; 
        }
        auto data = static_cast<const unsigned char*>(get_base());
        for (int i=start;i<=end;++i) {
            l.pushinteger(data[i-1]);
        }
        return {end-start+1};
    }

    lua::multiret buffer::lconcat(lua::state& l) {
        if (l.get_type(1)==lua::value_type::userdata) {
            auto self = lua::stack<buffer_ptr>::get(l, 1);
            if (!self) l.argerror(1, "need buffer");
            size_t size2 = 0;
            const void* data2 = l.checklstring(2, size2);
            std::vector<char> data;
            data.reserve(size2+self->get_len());
            data.resize(size2+self->get_len());
            memcpy(data.data(), self->get_base(), self->get_len());
            memcpy(data.data()+self->get_len(), data2, size2);
            l.pushlstring(data.data(), data.size());
            return {1};
        } else if (l.get_type(2)==lua::value_type::userdata) {
            auto self = lua::stack<buffer_ptr>::get(l, 2);
            if (!self) l.argerror(2, "need buffer");
            size_t size1 = 0;
            const void* data1 = l.checklstring(1, size1);
            std::vector<char> data;
            data.reserve(size1+self->get_len());
            data.resize(size1+self->get_len());
            memcpy(data.data(), data1, size1);
            memcpy(data.data()+size1, self->get_base(), self->get_len());
            l.pushlstring(data.data(), data.size());
            return {1};
        } else {
            l.argerror(1, "buffer concat");
        }
        return {0};
    }

    void buffer::lbind(lua::state& l) {
        lua::bind::function(l,"get_len",&buffer::get_len);
        lua::bind::function(l,"__len",&buffer::get_len);
        lua::bind::function(l,"__concat",&buffer::lconcat);
        lua::bind::function(l,"__tostring",&buffer::ltostring);
        lua::bind::function(l,"sub",&buffer::sub);
        lua::bind::function(l,"find",&buffer::lfind);
        lua::bind::function(l,"byte",&buffer::lbyte);
        lua::bind::function(l,"hex",&buffer::hex);
        lua::bind::function(l,"base64_encode",&buffer::base64_encode);
        lua::bind::function(l,"base64_decode",&buffer::base64_decode);
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
