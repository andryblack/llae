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

    lua::multiret buffer::lnew(lua::state& l) {
        size_t len = 0;
        auto base = l.checklstring(1,len);
        auto res = buffer::alloc(len);
        memcpy(res->get_base(),base,len);
        lua::push(l,std::move(res));
        return {1};
    }


    lua::multiret buffer::sub(lua::state& l) {
        if (get_len()==0) {
            l.pushstring("");
            return {1};
        }
        auto len = get_len();
        lua_Integer begin = l.checkinteger(2);
        lua_Integer end = l.optinteger(3,-1);
        if (begin < 0){
            begin = len + 1 + begin;
        }
        if (end < 0) {
            end = len + 1 + end;
        }
        if (begin < 1) {
            begin = 1;
        }
        if (end > len) {
            end = len;
        }
        if (begin > end) {
            l.pushstring("");
            return {1};
        }
        l.pushlstring(m_buf.base+begin-1,end-begin+1);
        return {1};
    }

    lua::multiret buffer::ltostring(lua::state& l) {
        l.pushlstring(m_buf.base,get_len());
        return {1};
    }

    lua::multiret buffer::leq(lua::state& l) {
        if (l.isstring(2)) {
            size_t len = 0;
            auto base = l.tolstring(2,len);
            if (len == get_len()) {
                l.pushboolean(memcmp(get_base(),base,len)==0);
                return {1};
            }
        } else {
            auto b = lua::stack<buffer_ptr>::get(l,2);
            if (b) {
                if (b.get()==this || (b->get_len() == get_len() && memcmp(get_base(),b->get_base(),get_len())==0)) {
                    l.pushboolean(false);
                    return {1};
                }
            }
        }
        l.pushboolean(false);
        return {1};
    }
    using uchar = unsigned char;

    static inline uchar decode_hex(lua::state& l,uchar ch) {
        if (ch>=uchar('0')&&ch<=uchar('9')) {
            return ch-uchar('0');
        }
        if (ch>=uchar('a')&&ch<=uchar('f')) {
            ch -= uchar('a');
            return ch + 0x0a;
        }
        if (ch>=uchar('A')&&ch<=uchar('F')) {
            ch -= uchar('A');
            return ch + 0x0a;
        }
        l.argerror(1, "non hex char");
        return 0;
    }
    lua::multiret buffer::hex_decode(lua::state& l) {
        const uchar* src = nullptr;
        size_t src_size = 0;
        if (l.isstring(1)) {
            src = reinterpret_cast<const uchar*>(l.tolstring(1,src_size));
        } else {
            auto self = lua::stack<buffer_ptr>::get(l, 1);
            if (!self) l.argerror(1, "need buffer or string");
            src = static_cast<const uchar*>(self->get_base());
            src_size = self->get_len();
        }
        if (src_size & 1) {
            l.argerror(1, "invalid source size");
        }
        size_t size = src_size/2;
        buffer_ptr buf(buffer::alloc(size));
        uchar* dst = static_cast<uchar*>(buf->get_base());
        
        for (size_t i=0;i<size;++i) {
            *dst = decode_hex(l,*src++) << 4;
            *dst |= decode_hex(l,*src++);
            ++dst;
        }
        lua::push(l,std::move(buf));
        return {1};
    }

    lua::multiret buffer::hex_encode(lua::state& l) {
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
        size_t size = src_size*2;
        buffer_ptr buf(buffer::alloc(size));
        char* dst = static_cast<char*>(buf->get_base());
        for (size_t i=0;i<src_size;++i) {
            *dst++ = hex_char[(*src>>4) & 0xf];
            *dst++ = hex_char[*src & 0xf];
            ++src;
        }
        lua::push(l,std::move(buf));
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
        lua::push(l,std::move(buf));
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
        lua::push(l,std::move(buf));
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
            const char* pos = static_cast<const char*>(memchr(start,*str,flen));
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

    buffer_ptr buffer::reverse() {
        auto res = buffer::alloc(get_len());
        size_t len = get_len();
        uchar* tail = static_cast<uchar*>(res->get_base()) + (len - 1);
        const uchar* front = static_cast<const uchar*>(get_base());
        for (size_t i=0;i<len;++i) {
            *tail = *front;
            ++front;
            --tail;
        }
        return res;
    }

    void buffer::self_reverse() {
        size_t len = get_len();
        if (len > 1) {
            uchar* front = static_cast<uchar*>(get_base());
            uchar* tail = front + (len - 1);
            size_t half = len / 2;
            for (size_t i=0;i<half;++i) {
                std::swap(*front,*tail);
                ++front;
                --tail;
            }
        }
    }

    lua::multiret buffer::lconcat(lua::state& l) {
        if (l.get_type(1)==lua::value_type::userdata &&
            l.get_type(2)==lua::value_type::userdata) {
            auto b1 = lua::stack<buffer_ptr>::get(l, 1);
            if (!b1) l.argerror(1, "need buffer");
            auto b2 = lua::stack<buffer_ptr>::get(l, 2);
            if (!b2) l.argerror(2, "need buffer");
            auto b = buffer::alloc(b1->get_len()+b2->get_len());
            memcpy(b->get_base(), b1->get_base(), b1->get_len());
            memcpy(static_cast<char*>(b->get_base())+b1->get_len(), b2->get_base(), b2->get_len());
            lua::push(l,std::move(b));
            return {1};
        } else if (l.get_type(1)==lua::value_type::userdata) {
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

    buffer_ptr buffer::get(lua::state& l,int idx) {
        buffer_ptr r = lua::stack<buffer_ptr>::get(l,idx);
        if (r) return r;
        /// @todo hold data on lua
        size_t size = 0;
        auto ptr = l.tolstring(idx,size);
        return hold(ptr,size);
    }

    void buffer::lbind(lua::state& l) {
        lua::bind::function(l,"new",&buffer::lnew);
        lua::bind::function(l,"get_len",&buffer::get_len);
        lua::bind::function(l,"__len",&buffer::get_len);
        lua::bind::function(l,"__concat",&buffer::lconcat);
        lua::bind::function(l,"__tostring",&buffer::ltostring);
        lua::bind::function(l,"__eq",&buffer::leq);
        lua::bind::function(l,"sub",&buffer::sub);
        lua::bind::function(l,"find",&buffer::lfind);
        lua::bind::function(l,"byte",&buffer::lbyte);
        lua::bind::function(l,"reverse", &buffer::reverse);
        lua::bind::function(l,"self_reverse", &buffer::self_reverse);
        lua::bind::function(l,"hex_decode",&buffer::hex_decode);
        lua::bind::function(l,"hex_encode",&buffer::hex_encode);
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
