#include "app.h"
#include "lua/state.h"
#include "lua/types.h"
#include "lua/value.h"
#include "meta/object.h"
#include "lua/metatable.h"
#include "uv/handle.h"
#include "logger.h"
#include <cstdint>
#include <psa/crypto.h>
#include "lua/bind.h"

namespace llae {

    static void show_lua_error(lua::state& l,lua::status e) {
        log::write(log::level::error,lua::get_error_message(l,e));
    }

    

    class app::lua_at_exit_handler : public at_exit_handler {
        lua::ref m_ref;
    public:
        explicit lua_at_exit_handler(lua::state& l,int arg) {
            l.pushvalue(arg);
            m_ref.set(l);
        }
        virtual void at_exit( app& app , int arg) override {
            auto& l{app.lua()};
            m_ref.push(l);
            m_ref.reset(l);
            l.pushinteger(arg);
            auto res = l.pcall(1,0,0);
            if (res != lua::status::ok) {
                show_lua_error(l,res);
            }
        }
    };

    int app::at_panic(lua_State* L) {
        log::write(log::level::error,"PANIC: ");
        lua::state l(L);
        lua::value err(l,-1);
        if (err.is_string()) {
            log::write(log::level::error,err.tostring());
        } else {
            log::write(log::level::error,"unknown");
        }
        app::get(l).process_error(lua::status::panic);
        return 0;
    }

    app::app(uv_loop_t* l,bool need_signal) : m_loop(l) {
        psa_crypto_init();
        *static_cast<app**>(lua_getextraspace(m_lua.native())) = this;
        uv_loop_set_data(m_loop.native(),this);
        m_lua.open_libs();
        lua_atpanic(lua().native(),&app::at_panic);

        lua::register_meta_object_metatable(lua());
        if (need_signal) {
            m_stop_sig.reset( new uv::signal(loop()) );
            m_stop_sig->start_oneshot(SIGINT,[this](int signum) {
                this->process_at_exit(signum);
                this->loop().stop();
            });
            m_stop_sig->unref();
        }
    }

    void app::cancel_signal() {
        if (m_stop_sig) {
            m_stop_sig->stop();
            m_stop_sig->close();
            m_stop_sig.reset();
        }
    }

    app::~app() {
        uv_loop_set_data(m_loop.native(),nullptr);
    }

    app& app::get(lua_State* L) {
        assert(L);
        return **static_cast<app**>(lua_getextraspace(L));
    }

    bool app::closed(uv_loop_t* l) {
        return uv_loop_get_data(l) == nullptr;
    }
    app& app::get(uv_loop_t* l) {
        assert(l);
        assert(!closed(l));
        return *static_cast<app*>(uv_loop_get_data(l));
    }

    void app::at_exit(lua::state& l,int arg) {
        common::intrusive_ptr<lua_at_exit_handler> h{new lua_at_exit_handler(l,arg) };
        m_at_exit.emplace_back(std::move(h));
    }

    void app::process_at_exit(int res) {
        for (auto& h:m_at_exit) {
            h->at_exit(*this,res);
        }
        m_at_exit.clear();
    }

    static void stop_walk_cb(uv_handle_t* handle,void* arg) {
        if (handle->data) {
            uv::handle* h = static_cast<uv::handle*>(handle->data);
            h->close();
        }
    }
    void app::close() {
        m_lua.close();
        if (m_stop_sig) {
            m_stop_sig->close();
        }
        m_stop_sig.reset();
    }
    void app::end_run(int res) {
        //std::cout << "app::run end " << v << std::endl;
        close();
        
        if (res != 0) {
            while(uv_run(loop().native(),UV_RUN_NOWAIT)) {
                uv_sleep(100);
                uv_walk(loop().native(),stop_walk_cb,0);
            }
        }
    }
    int app::run() {
        int v = m_loop.run(UV_RUN_DEFAULT);
        end_run(v);
        return m_res;
    }

    void app::stop(int res) {
        process_at_exit(res);
        m_res = res;
        loop().stop();
    }

    void app::process_error(lua::status e) {
        log::flush();
        if (m_error_handler) {
            m_error_handler->handle_error(*this,e);
        } else {
            stop(1);
        }
    }

    void app::show_error(lua::state& l,lua::status e, bool pop) {
        auto& self(get(l));
        show_lua_error(l,e);
        lua::state& ms(self.lua());
        luaL_traceback(ms.native(),l.native(),NULL,1);
        log::write(log::level::error,lua::value(ms,-1).tostring());
        ms.pop(1);
        self.process_error(e);
    }

    void app::lua_resume(lua::state& l) {
        l.checktype(1,lua::value_type::thread);
        auto n = l.gettop();
        auto t = l.tothread(1);
        for (int i=2;i<=n;++i) {
            l.pushvalue(i);
            t.xmove(l,1);
        }
        auto s = t.resume(l,n-1);
        if (s!=lua::status::yield && s!=lua::status::ok) {
            show_error(t,s);
        }
    }
    

   
}


