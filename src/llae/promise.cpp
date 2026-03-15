#include "promise.h"
#include "app.h"
#include "lua/bind.h"

META_OBJECT_INFO(llae::promise_base,meta::object);

namespace llae {

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

    lua::multiret promise_base::lawait(lua::state& l) {
        if (!is_pending()) {
            return {push(l)};
        }
        if (!l.isyieldable()) {
            l.error("promise await is async");
        }
        {
            lua::ref cont;
            l.pushthread();
            cont.set(l);
            app& a = app::get(l);
            auto func = [cont_m = std::move(cont),&a]  (promise_base& p) mutable {
                auto l = a.lua();
                if (!l.native()) {
                    cont_m.release();
                    return;
                }
                l.checkstack(2);
                cont_m.push(l);
                cont_m.reset(l);
                auto toth = l.tothread(-1);
                toth.checkstack(3);
                int nargs = p.push(toth);
                auto s = toth.resume(l,nargs);
                if (s != lua::status::ok && s != lua::status::yield) {
                    app::show_error(toth,s);
                }
            };
            await( std::move(func));
        }
        l.yield(0);
        return {0};
    }


    void promise_base::lbind(lua::state &l) {
        lua::bind::function(l,"await",&promise_base::lawait);
    }
}