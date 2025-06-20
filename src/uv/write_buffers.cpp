#include "write_buffers.h"
#include "lua/stack.h"

namespace uv {

   
    bool write_buffers::put_one(lua::state& l) {
        auto buf = lua::stack<llae::buffer_base_ptr>::get(l, -1);
        if (buf) {
            m_refs.emplace_back();
            m_refs.back().set(l);
            auto base = const_cast<char*>(static_cast<const char*>(buf->get_base()));
            m_bufs.emplace_back(uv_buf_t{base,buf->get_len()});
        } else {
            size_t size;
            
            const char* val = l.tolstring(-1,size);
            if (val && size !=0) {
                m_refs.emplace_back();
                m_refs.back().set(l);
                m_bufs.push_back(uv_buf_init(const_cast<char*>(val),static_cast<unsigned int>(size)));
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
            m_bufs.reserve(m_bufs.size()+tl);
            m_refs.reserve(m_refs.size()+tl);
            for (size_t j=0;j<tl;++j) {
                l.rawgeti(-1,int(j+1));
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

    bool write_buffers::putm(lua::state& s,int base) {
        auto top = s.gettop();
        for (int i=base;i<=top;++i) {
            s.pushvalue(i);
            if (!put(s)) {
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
