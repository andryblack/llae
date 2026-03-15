#pragma once

#include "common/intrusive_ptr.h"
#include <coroutine>
#include "llae/error.h"
#include "promise.h"

namespace llae {

    template <typename R>
    class promise_awaitable {
    public:
        using result_type = result<R>;
        using co_promise_type = promise<result_type>;
        using promise_type_ptr = common::intrusive_ptr<co_promise_type>;
        using storage = promise_type_ptr;
    private:
        storage m_promise;
    public:
        explicit promise_awaitable(storage&& s) : m_promise(s) {}
        bool await_ready() { 
            return m_promise->is_resolved();
        }
        bool await_suspend(std::coroutine_handle<> suspended) {
            if (await_ready()) {
                return false;
            }
            m_promise->await( [suspended](promise_base& p) {
                suspended.resume();
            });
            return true;
        }
        result_type await_resume() {
            return m_promise->get_result();
        }
        
    };

    template< typename T >
    struct co_result_promise_type {
        using promise_type_ptr = result_promise_ptr<T>;
        promise_type_ptr res = common::make_intrusive<result_promise<T>>();
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
        
        void return_value(result<T>&& r) {
            res->set_result(std::move(r));
        }
        void return_value(error_ptr&& r) {
            res->set_result(std::move(r));
        }
        void return_value(const error_ptr& r) {
            res->set_result(result<T>(r));
        }
        promise_type_ptr get_return_object() {
            return res;
        }
    };

    template <typename R>
    static promise_awaitable<R> operator co_await ( result_promise_ptr<R> r) {
        return promise_awaitable(std::move(r));
    }
}

namespace std {
    template<typename R,typename... Args>
    struct coroutine_traits<llae::result_promise_ptr<R>,Args...> {
        using promise_type = llae::co_result_promise_type<R>;
    };
}
