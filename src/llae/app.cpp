#include "app.h"
#include "lua/value.h"
#include "meta/object.h"
#include "lua/bind.h"
#include "uv/handle.h"

#include <iostream>

namespace llae {

    static int at_panic(lua_State* L) {
        std::cout << "PANIC: ";
        lua::state l(L);
        lua::value err(l,-1);
        if (err.is_string()) {
            std::cout << err.tostring() << std::endl;
        } else {
            std::cout << "unknown" << std::endl;
        }
        app::get(L).stop();
        return 0;
    }

    app::app(uv_loop_t* l) : m_loop(l) {
        *static_cast<app**>(lua_getextraspace(m_lua.native())) = this;
        uv_loop_set_data(m_loop.native(),this);
        m_lua.open_libs();
        lua_atpanic(lua().native(),&at_panic);
        lua::bind::object<meta::object>::register_metatable(lua());
        m_stop_sig.reset( new uv::signal(loop()) );
        m_stop_sig->start_oneshot(SIGINT,[this]() {
             this->stop();
        });
        m_stop_sig->unref();
    }

    app::~app() {
    }

    app& app::get(lua_State* L) {
        assert(L);
        return **static_cast<app**>(lua_getextraspace(L));
    }

    app& app::get(uv_loop_t* l) {
        assert(l);
        return *static_cast<app*>(uv_loop_get_data(l));
    }

    static void stop_walk_cb(uv_handle_t* handle,void* arg) {
        if (uv_is_active(handle)) {
            if (handle->data) {
                uv::handle* h = static_cast<uv::handle*>(handle->data);
                h->close();
            }
        }
    }
    void app::run() {
        int v = m_loop.run(UV_RUN_DEFAULT);
        //std::cout << "app::run end " << v << std::endl;
        if (v != 0) {
            while(uv_run(loop().native(),UV_RUN_NOWAIT)) {
                uv_sleep(100);
                uv_walk(loop().native(),stop_walk_cb,0);
            }
        }
        m_lua.close();
        return;
    }

    void app::stop() {
        //uv_print_all_handles(uv_default_loop(), stderr);
        m_loop.stop();
        m_stop_sig.reset();
    }

    void app::show_error(lua::state& l,lua::status e) {
        switch(e) {
            case lua::status::yield:
                std::cout << "YIELD:\t";
                break;
            case lua::status::errun:
                std::cout << "ERRRUN:\t";
                break;
            case lua::status::errsyntax:
                std::cout << "ERRSYNTAX:\t";
                break;
            case lua::status::errmem:
                std::cout << "ERRMEM:\t";
                break;
            case lua::status::errgcmm:
                std::cout << "ERRGCMM:\t";
                break;
            case lua::status::errerr:
                std::cout << "ERRERR:\t";
                break;
            default:
                std::cout << "UNKNOWN:\t";
                break;
        };
        lua::value err(l,-1);
        if (err.is_string()) {
            std::cout << err.tostring() << std::endl;
        } else {
            std::cout << "unknown" << std::endl;
        }
    }

    static int lua_stop(lua_State* L) {
        app::get(L).stop();
        return 0;
    }
    
}


int luaopen_llae(lua_State* L) {
    lua::state s(L);
    luaL_Reg reg[] = {
        { "stop", llae::lua_stop },
        { NULL, NULL }
    };
    lua_newtable(L);
    luaL_setfuncs(L, reg, 0);
    return 1;
}
