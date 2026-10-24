#include "promise.h"
#include "app.h"
#include "lua/bind.h"

META_OBJECT_INFO(llae::promise_base,meta::object);

namespace llae {

    // Called by Lua when the coroutine is resumed: pushes the resolved value(s)
    // and returns their count. Runs inside lua_resume, so RAII works here.
    // 'ctx' is the promise kept alive by the reference handed over at yield
    // time (the frame that started the await is discarded by lua_yieldk).
    static int await_continuation(lua_State* L, int /*status*/, lua_KContext ctx) {
        auto* promise = reinterpret_cast<promise_base*>(ctx);
        lua::state l(L);
        l.checkstack(8);
        int count = promise->push_result(l);
        // the resolver still holds a reference while it calls set_result, so
        // this never destroys the promise in the middle of its own resolve()
        promise->remove_ref();
        return count;
    }

    void promise_base::resolve() {
        if (!m_resolved) {
            m_resolved = true;
            auto callbacks = std::move(m_callbacks);
            m_callbacks.clear();
            for (auto& cb:callbacks) {
                cb(*this);
            }
        }
    }

    int await_promise(lua::state& l,promise_base_ptr promise) {
        if (!promise) {
            l.error("await without promise");
        }
        if (!promise->is_pending()) {
            return promise->push_result(l);
        }
        if (!l.isyieldable()) {
            l.error("promise await is async");
        }
        lua::ref cont;
        l.pushthread();
        cont.set(l);
        app& a = app::get(l);
        // the result is pushed by await_continuation, so the coroutine is
        // resumed without arguments
        auto func = [cont_m = std::move(cont),&a] (promise_base& /*p*/) mutable {
            auto l = a.lua();
            if (!l.native()) {
                cont_m.release();
                return;
            }
            l.checkstack(2);
            cont_m.push(l);
            auto toth = l.tothread(-1);
            l.pop(1);// thread, at cont_m
            toth.checkstack(4);
            auto s = toth.resume(l,0);
            if (s != lua::status::ok && s != lua::status::yield) {
                app::show_error(toth,s);
            }
            cont_m.reset(l);
        };
        // The frame below is discarded by lua_yieldk (no destructors run), so
        // the reference to the promise is handed over to the continuation
        // instead of being owned by a local: discard() transfers it without
        // releasing, await_continuation() releases it after the resume.
        auto* raw = promise.discard();
        raw->await(std::move(func));
        // does not return: the continuation below provides the results when
        // the coroutine is resumed
        l.yieldk(0,reinterpret_cast<lua_KContext>(raw),&await_continuation);
        return 0;
    }

    lua::multiret promise_base::lawait(lua::state& l) {
        return {await_promise(l,promise_base_ptr(this))};
    }


    void promise_base::lbind(lua::state &l) {
        lua::bind::function(l,"await",&promise_base::lawait);
    }
}
