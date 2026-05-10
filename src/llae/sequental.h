#pragma once
#include "common/intrusive_ptr.h"
#include "common/inplace_function.h"
#include "llae/result.h"
#include "llae/error.h"
#include "llae/work.h"
#include <utility>

namespace llae {

    class loop;

    class sequental {
    private:
        const char* m_current_op = nullptr;
    public:
        error_ptr start_op(const char* new_op);
        void end_op(const char* name);
    };

    class sequental_scope {
    private:
        sequental& m_seq;
        const char* m_name = nullptr;
    public:
        explicit sequental_scope(sequental& s) : m_seq(s) {}
        ~sequental_scope() {
            if (m_name) {
                m_seq.end_op(m_name);
            }
        }
        error_ptr start(const char* op);
    };

    template <typename R,typename T,typename Base>
    struct sequental_work_hold_base {
        common::intrusive_ptr<T> obj;
        using seq_ptr_t = sequental T::*;
        seq_ptr_t seq = nullptr;
        const char* name = nullptr;
        Base base;
        template <typename...Args>
        explicit sequental_work_hold_base(T* obj,seq_ptr_t seq,const char* name,Args...args) : obj(obj),seq(seq),name(name),base(std::forward<Args>(args)...) {}
        
        error_ptr try_start() {
            if (!obj) {
                return string_error::create("object releassed");
            }
            auto res = (obj.get()->*seq).start_op(name);
            if (res) {
                obj.reset();
                return std::move(res);
            }
            if constexpr (work_hold_traits::has_try_start<Base>::value) {
                if (auto err = base.try_start()) {
                    return std::move(err);
                }
            }
            return error_ptr{};
        }
        void reset() {
            base.reset();
            if (obj) {
                (obj.get()->*seq).end_op(name);
                obj.reset();
                seq = nullptr;
                name = nullptr;
            };
        }

        void release(loop& a) {
            if constexpr (work_hold_traits::has_release<Base>::value) {
                base.release(a);
            }
        }
    };

    template <typename R,typename T,size_t Size>
    struct sequental_method_work_hold : sequental_work_hold_base<R,T,common::inplace_function<result<R>(T&),Size>> {
        using Base = sequental_work_hold_base<R,T,common::inplace_function<result<R>(T&),Size>>;
        template <typename...Args>
        explicit sequental_method_work_hold(T* obj,Base::seq_ptr_t seq,const char* name,Args...args) : Base(obj,seq,name,std::forward<Args>(args)...) {}
        result<R> operator () () {
            if (!this->obj) {
                return string_error::create("object releassed");
            }
            return this->base(*this->obj);
        }
    };

    template <typename R,typename T,size_t Size = 4>
    using sequental_method_work = function_work<R,sequental_method_work_hold<R,T,Size>>;

}