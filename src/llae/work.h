#pragma once

#include "common/intrusive_ptr.h"
#include "common/inplace_function.h"
#include "llae/error.h"
#include "meta/object.h"
#include "result.h"
#include "promise.h"
#include "loop.h"
#include <algorithm>
#include <type_traits>
#include <utility>

namespace llae {


    class work_base : public meta::object {
        META_OBJECT
    public:
        virtual void do_work() = 0;
    protected:
        error_ptr schedule_work(loop& a);
    private:
        class work_impl;
        virtual void on_after_work(loop& a,error_ptr e) = 0;
    };

    template <typename R>
    class work : public work_base {
    public:
        using result_type = result<R>;
        using promise_type = promise<result_type>;
        using promise_ptr = common::intrusive_ptr<promise_type>;
        using this_ptr = common::intrusive_ptr<work<R>>;
    protected:
        virtual result_type after_work(loop& a) = 0;
        virtual void release(loop& a) {}
    private:
        promise_ptr m_promise;

        virtual void on_after_work(loop& a,error_ptr e) override final {
            if (m_promise && !m_promise->is_resolved()) {
                if (e) {
                    release(a);
                    m_promise->set_result(result_type(std::move(e)));
                    m_promise.reset();
                } else {
                    auto res = after_work(a);
                    release(a);
                    m_promise->set_result(std::move(res));
                    m_promise.reset();
                }
                return;
            }
            release(a);
            m_promise.reset();
        }
    public:
        static promise_ptr forward_error(error_ptr&& e) {
            return result_promise_forward_error<R>(std::move(e));
        }
        promise_ptr async_run(loop& a) {
            if (m_promise) {
                return make_result_promise_string_error<R>("already scheduled");
            }
            m_promise = common::make_intrusive<promise_type>();
            auto err = schedule_work(a);
            if (err) {
                release(a);
                m_promise.reset();
                return forward_error(std::move(err));
            }
            return m_promise;
        }
    };

    template <typename R, size_t Size=4>
    using default_function_work_hold = common::inplace_function<result<R>(),Size>;

    template <typename R,typename T>
    struct method_work_hold {
        using func_t = result<R> (T::*)();
        common::intrusive_ptr<T> ptr;
        func_t func;
        explicit method_work_hold(common::intrusive_ptr<T>&& ptr,func_t func) : ptr(std::move(ptr)),func(func) {}
        void reset() {
            ptr.reset();
        }
        result<R> operator () () {
            return (ptr.get()->*func)();
        }
    };

    struct work_hold_traits {
        template <class T, class = void>
        struct has_try_start: std::false_type {};
        template <class T>
        struct has_try_start<T, std::void_t<decltype(&T::try_start)>> : std::true_type {};

        template <class T, class = void>
        struct has_release: std::false_type {};
        template <class T>
        struct has_release<T, std::void_t<decltype(&T::release)>> : std::true_type {};
    };

    template <typename R,typename Hold = default_function_work_hold<R> >
    class function_work : public work<R> {
    private:
        Hold m_hold;
        std::optional<result<R>> m_result;

        

        error_ptr try_start() {
            if constexpr (work_hold_traits::has_try_start<Hold>::value) {
                return m_hold.try_start();
            } else {
                return error_ptr{};
            }
        }
        void release_hold(loop& a) {
            if constexpr (work_hold_traits::has_release<Hold>::value) {
                m_hold.release(a);
            }
        }
    public:
        using promise_ptr = work<R>::promise_ptr;
        template<typename...Args>
        explicit function_work(Args...args) : m_hold(std::forward<Args>(args)...) {}
        virtual void do_work() override final {
            m_result = m_hold();
        }
        virtual result<R> after_work(loop&) override final {
            if (!m_result.has_value()) {
                return string_error::create("work failed");
            }
            return std::move(*m_result);
        } 
        virtual void release(loop& a) override {
            m_result.reset();
            release_hold(a);
            m_hold.reset();
        }

        template<typename...Args>
        static promise_ptr start(loop& a,Args...args) {
            auto w = common::make_intrusive<function_work<R,Hold>>(std::forward<Args>(args)...);
            if (auto err = w->try_start()) {
                w->release(a);
                return llae::result_promise_forward_error<R>(std::move(err));
            }
            return w->async_run(a);
        }
    };

}