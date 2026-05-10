#pragma once

#include "common/intrusive_ptr.h"
#include "llae/error.h"
#include "meta/object.h"

#include <memory>
#include <string_view>
#include <vector>
#include "lua/state.h"
#include "lua/stack.h"
#include "common/optional_storage.h"
#include "result.h"
#include "common/inplace_function.h"

namespace llae {

    
    class promise_base : public meta::object {
        META_OBJECT
    protected:
        using this_type = promise_base;
        using this_ptr = common::intrusive_ptr<this_type>;
        virtual int push(lua::state& l) = 0;
        
        void resolve();
        template<typename Func>
        void schedule(Func&& cb) {
            m_callbacks.emplace_back(std::move(cb));
        }
    private:
        std::vector<common::inplace_function<void(promise_base&)>> m_callbacks;
        bool m_resolved = false;
    public:
        promise_base() : meta::object() {}
        virtual ~promise_base() override {}

        bool is_pending() const { return !m_resolved; }
        bool is_resolved() const { return m_resolved; }
        
        lua::multiret lawait(lua::state& l);

        template <typename F>
        void await(F&& callback) {
            if (is_pending()) {
                schedule(std::move(callback));
            } else {
                callback(*this);
            }
        }
        static void lbind(lua::state& l);
    };
    using promise_base_ptr = common::intrusive_ptr<promise_base>;

    template <typename R>
    class promise : public promise_base {
    public:
        using this_type = promise<R>;
        using this_ptr = common::intrusive_ptr<this_type>;
    private:
        using result_storage = typename common::optional_storage<R>::type;
        result_storage m_result;
    protected:
        int push(lua::state& l) override {
            return lua::stack<R>::push(l,get_result());
        }
    public:
        promise() : m_result() {}
        promise(R&& result) : m_result(std::move(result)) {
            resolve();
        }

        void set_result(R&& result) { 
            m_result = std::move(result); 
            resolve();
        }
  
        R& get_result() { return common::optional_storage<R>::get(m_result); }
        const R& get_result() const { return common::optional_storage<R>::get(m_result); }
        template <typename F>
        void await(F&& callback) {
            if (is_pending()) {
                schedule([cb = std::move(callback)](promise_base& p){
                    cb(static_cast<this_type&>(p));
                });
            } else {
                callback(*this);
            }
        }
    };

    template <typename R>
    using promise_ptr = common::intrusive_ptr<promise<R>>;

    template <typename R>
    using result_promise = promise<result<R>>;

    template <typename R>
    using result_promise_ptr = common::intrusive_ptr<result_promise<R>>;

    template <typename R>
    static inline result_promise_ptr<R> make_result_promise_string_error(std::string_view msg) {
        return common::make_intrusive<result_promise<R>>(result<R>(string_error::create(msg)));
    }

    template <typename R>
    static inline result_promise_ptr<R> result_promise_forward_error(error_ptr&& e) {
        return common::make_intrusive<result_promise<R>>(result<R>(std::move(e)));
    }
    
}