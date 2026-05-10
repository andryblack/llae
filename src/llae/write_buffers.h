#pragma once

#include "common/intrusive_ptr.h"
#include "llae/buffer.h"
#include "llae/result.h"
#include "lua/ref.h"
#include "lua/stack.h"
#include <vector>
#include <cstdlib>
#include "llae/work.h"
#include "llae/sequental.h"
#include "llae/app.h"


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

    template <typename R,typename T>
    struct method_write_buffers_work_hold_base {
        write_buffers buffers;
        using func_t = result<R> (T::*)(const write_buffers&);
        func_t func;
        explicit method_write_buffers_work_hold_base(write_buffers&& buffers,func_t func) : buffers(std::move(buffers)),func(func) {}
        void release(loop& a) {
            buffers.reset(llae::app::get(a).lua());
        }
        void reset() {
        }
        result<R> call(T& ptr) {
            return (ptr.*func)(buffers);
        }
    };

    template <typename R,typename T>
    struct method_write_buffers_work_hold : method_write_buffers_work_hold_base<R,T> {
        using Base = method_write_buffers_work_hold_base<R,T>;
        common::intrusive_ptr<T> obj;
        explicit method_write_buffers_work_hold(write_buffers&& buffers,T* ptr,Base::func_t func) : Base(std::move(buffers),func),obj(ptr) {}
        void reset() {
            obj.reset();
        }
        result<R> operator () () {
            if (!obj) {
                return string_error::create("oject released");
            }
            return this->call(*obj);
        }
    };

    template <typename R,typename T>
    using method_write_buffers_work = function_work<R,method_write_buffers_work_hold<R,T>>;

    template <typename R,typename T>
    struct sequental_method_write_buffers_work_hold : sequental_work_hold_base<R,T,method_write_buffers_work_hold_base<R,T>> {
        using Base = sequental_work_hold_base<R,T,method_write_buffers_work_hold_base<R,T>>;
        using HoldBase = method_write_buffers_work_hold_base<R,T>;
        explicit sequental_method_write_buffers_work_hold(write_buffers&& buffers,T* ptr,Base::seq_ptr_t seq,const char* name,HoldBase::func_t func) : Base(ptr,seq,name,std::move(buffers),func) {}
        result<R> operator () () {
            if (!this->obj) {
                return string_error::create("oject released");
            }
            return this->base.call(*this->obj);
        }
    };

    template <typename R,typename T>
    using sequental_method_write_buffers_work = function_work<R,sequental_method_write_buffers_work_hold<R,T>>;
    
}


