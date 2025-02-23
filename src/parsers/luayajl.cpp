
extern "C" {
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
}

#include <vector>
#include <cassert>
#include <string>
#include "uv/buffer.h"
#include "lua/state.h"
#include "lua/bind.h"
#include "json.h"

static int json_array_marker = 0;
    
struct parse_context {
    lua::state& l;
    int stack_depth = 0;
    bool mark_arrays = false;
    struct state_t {
        enum type {
            array,
            map
        } type;
        int count;
    };
    std::vector<state_t> state;
    void on_pre_value() {
        if (!state.empty()) {
            if (state.back().type == state_t::array) {
                ++state.back().count;
                l.pushinteger(state.back().count);
                ++stack_depth;
            }
        }
    }
    void on_value(bool pop = false) {
        if (pop) {
            assert(!state.empty());
            state.pop_back();
        }
        if (!state.empty()) {
            if (state.back().type == state_t::map) {
                assert(l.istable(-3));
                assert(l.isstring(-2));
                l.settable(-3);
                stack_depth-=2;
            } else if (state.back().type == state_t::array) {
                assert(l.istable(-3));
                assert(l.isnumber(-2));
                l.settable(-3);
                stack_depth-=2;
            }
        }
        
    }
};
    
static int yajl_parse_null(void * ctx) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    c->l.pushnil();
    ++c->stack_depth;
    c->on_value();
    return 1;
}
static int yajl_parse_boolean(void * ctx, int boolVal) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    c->l.pushboolean(boolVal);
    ++c->stack_depth;
    c->on_value();
    return 1;
}
static int yajl_parse_integer(void * ctx, long long integerVal) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    c->l.pushinteger(integerVal);
    ++c->stack_depth;
    c->on_value();
    return 1;
}
static int yajl_parse_double(void * ctx, double doubleVal) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    c->l.pushnumber(doubleVal);
    ++c->stack_depth;
    c->on_value();
    return 1;
}

/** strings are returned as pointers into the JSON text when,
 * possible, as a result, they are _not_ null padded */
static int yajl_parse_string(void * ctx, const unsigned char * stringVal,
                             size_t stringLen) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    c->l.pushlstring(reinterpret_cast<const char*>(stringVal),stringLen);
    ++c->stack_depth;
    c->on_value();
    return 1;
}

static int yajl_parse_start_map(void * ctx) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    c->l.checkstack(5);
    c->l.newtable();
    ++c->stack_depth;
    parse_context::state_t s;
    s.type = parse_context::state_t::map;
    s.count = 0;
    c->state.push_back(s);
    return 1;
}
static int yajl_parse_map_key(void * ctx, const unsigned char * key,
                              size_t stringLen) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->l.pushlstring(reinterpret_cast<const char*>(key),stringLen);
    ++c->stack_depth;
    return 1;
}
static int yajl_parse_end_map(void * ctx) {
    parse_context* c = static_cast<parse_context*>(ctx);
    assert(!c->state.empty());
    assert(c->state.back().type==parse_context::state_t::map);
    c->on_value(true);
    return 1;
}

static int yajl_parse_start_array(void * ctx) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->l.checkstack(5);
    c->on_pre_value();
    c->l.newtable();
    if (c->mark_arrays) {
        c->l.getfield(LUA_REGISTRYINDEX,"json_array");
        c->l.setmetatable(-2);
    }

    ++c->stack_depth;
    parse_context::state_t s;
    s.type = parse_context::state_t::array;
    s.count = 0;
    c->state.push_back(s);
    return 1;
}
static int yajl_parse_end_array(void * ctx) {
    parse_context* c = static_cast<parse_context*>(ctx);
    assert(!c->state.empty());
    assert(c->state.back().type==parse_context::state_t::array);
    c->on_value(true);
    return 1;
}

static void fill_parse_callbacks( yajl_callbacks& cb ) {
    cb.yajl_null = &yajl_parse_null;
    cb.yajl_boolean = &yajl_parse_boolean;
    cb.yajl_integer = &yajl_parse_integer;
    cb.yajl_double = &yajl_parse_double;
    cb.yajl_number = 0;
    cb.yajl_string = &yajl_parse_string;
    cb.yajl_start_map = &yajl_parse_start_map;
    cb.yajl_map_key = &yajl_parse_map_key;
    cb.yajl_end_map = &yajl_parse_end_map;
    cb.yajl_start_array = &yajl_parse_start_array;
    cb.yajl_end_array = &yajl_parse_end_array;
}
    
static lua::multiret json_decode(lua::state& l) {

    auto data = uv::buffer_view::get(l,1,true);
    bool safe = false;

    if (l.isboolean(2)) {
        safe = l.toboolean(2);
    } else if (l.istable(2)) {
        l.getfield(2,"safe");
        safe = l.toboolean(-1);
        l.pop(1);
    }
   
    yajl_callbacks cb;
    fill_parse_callbacks(cb);
    parse_context ctx = {l};
    ctx.mark_arrays = false;
    if (l.isboolean(3)) {
        ctx.mark_arrays = l.toboolean(3);
    } else if (l.istable(2)) {
        l.getfield(2,"mark_arrays");
        ctx.mark_arrays = l.toboolean(-1);
        l.pop(1);
    }
    
    yajl_handle h = yajl_alloc(&cb, 0, &ctx);
    
    if (l.istable(2)) {
        l.getfield(2, "validate_utf");
        bool validata = l.toboolean(-1);
        l.pop(1);
        yajl_config(h,yajl_dont_validate_strings,validata?0:1);
        l.getfield(2, "allow_comments");
        bool allow = l.toboolean(-1);
        yajl_config(h,yajl_allow_comments,allow?1:0);
        l.pop(1);
    } else {
        yajl_config(h,yajl_allow_comments,1);
    }
    yajl_status s = yajl_parse(h,reinterpret_cast<const unsigned char * >(data.get_base()),data.get_len());
    if (s == yajl_status_ok) {
        s = yajl_complete_parse(h);
    }
    if (s!=yajl_status_ok) {
        unsigned char* err_text = yajl_get_error(h, 1,reinterpret_cast<const unsigned char * >(data.get_base()),data.get_len());
        l.pop(ctx.stack_depth);
        if (safe) {
            l.pushnil();
        }
        l.pushstring(reinterpret_cast<const char*>(err_text));
        yajl_free_error(h,err_text);
        yajl_free(h);
        if (safe) {
            return {2};
        } else {
            l.error();
            return {1};
        }
    }
    yajl_free(h);
    return {ctx.stack_depth};
}

    
struct encode_context {
    std::string data;
};
static void yajl_encode_print(void * ctx,const char * str,size_t len) {
    encode_context* c = static_cast<encode_context*>(ctx);
    c->data.append(str,len);
}
static void do_json_encode(lua_State* L,int indx,yajl_gen g);
    
static void do_json_encode_string(lua_State* L,int idx, yajl_gen g) {
    size_t len = 0;
    const char* str = 0;
    str = lua_tolstring(L,idx,&len);
    if (!str || len == 0) {
        yajl_gen_string(g, reinterpret_cast<const unsigned char*>("unknown"),7 );
    } else {
        yajl_gen_string(g, reinterpret_cast<const unsigned char*>(str),len );
    }
}

static void do_json_encode_table(lua_State* L,int indx,yajl_gen g) {
    assert(lua_istable(L, indx));
    int count = 0;
    
    lua_checkstack(L,10);
    lua_pushvalue(L, indx);
    bool is_array = false;
    if (lua_getmetatable(L,-1)) {
        if (lua_getfield(L,-1,"__json_type")==LUA_TLIGHTUSERDATA) {
            is_array = lua_touserdata(L,-1) == &json_array_marker;
        }
        lua_pop(L,2);
    }

    bool only_integers = true;
    if (!is_array) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            /* uses 'key' (at index -2) and 'value' (at index -1) */
            if (lua_isnumber(L, -2)) {
                lua_Integer i = lua_tointeger(L, -2);
                lua_Number n = lua_tonumber(L, -2);
                if ( lua_Number(i) == n ) {
                    ++count;
                    if (count!=i) {
                        only_integers = false;
                    }
                } else {
                    only_integers = false;
                }
            } else {
                only_integers = false;
            }
            
            if (!only_integers) {
                lua_pop(L, 2);
                break;
            }
            /* removes 'value'; keeps 'key' for next iteration */
            lua_pop(L, 1);
        }
    }
    if ((only_integers && count!=0) || is_array) {
        yajl_gen_array_open(g);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            /* uses 'key' (at index -2) and 'value' (at index -1) */
            do_json_encode(L, -1, g);
            /* removes 'value'; keeps 'key' for next iteration */
            lua_pop(L, 1);
        }
        yajl_gen_array_close(g);
    } else {
        yajl_gen_map_open(g);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            /* uses 'key' (at index -2) and 'value' (at index -1) */
            int t = lua_type(L, -2);
            if (t == LUA_TSTRING) {
                do_json_encode_string(L,-2,g);
            } else {
                lua_pushvalue(L,-2);
                do_json_encode_string(L,-1,g);
                lua_pop(L,1);
            }
            
            do_json_encode(L, -1, g);

            assert(lua_type(L, -2) == t);
            /* removes 'value'; keeps 'key' for next iteration */
            lua_pop(L, 1);
        }
        yajl_gen_map_close(g);
    }
    lua_pop(L, 1);
}
    
static void do_json_encode(lua_State* L,int indx,yajl_gen g) {
    int type = lua_type(L, indx);
    if (type == LUA_TNIL) {
        yajl_gen_null(g);
    } else if (type == LUA_TBOOLEAN) {
        yajl_gen_bool(g, lua_toboolean(L, indx));
    } else if (type == LUA_TNUMBER) {
        lua_Integer i = lua_tointeger(L, indx);
        lua_Number n = lua_tonumber(L, indx);
        if ( lua_Number(i) == n ) {
            yajl_gen_integer(g, i);
        } else {
            yajl_gen_double(g, n);
        }
    } else if (type == LUA_TSTRING) {
        size_t len = 0;
        const char* str = lua_tolstring(L,indx,&len);
        yajl_gen_string(g, reinterpret_cast<const unsigned char*>(str), len);
    } else if (type == LUA_TFUNCTION) {
        yajl_gen_string(g, reinterpret_cast<const unsigned char*>("function"), 8);
    } else if (type == LUA_TTHREAD) {
        yajl_gen_string(g, reinterpret_cast<const unsigned char*>("thread"), 6);
    } else if (type == LUA_TLIGHTUSERDATA) {
        yajl_gen_string(g, reinterpret_cast<const unsigned char*>("ldata"), 5);
    }  else if (type == LUA_TUSERDATA) {
        yajl_gen_string(g, reinterpret_cast<const unsigned char*>("data"), 4);
    } else if (type == LUA_TTABLE) {
        do_json_encode_table(L,indx,g);
    } else {
        /// unknown;
        assert(false);
        yajl_gen_null(g);
    }
}
    
static int json_encode(lua_State* L) {
    encode_context ctx;
    yajl_gen g = yajl_gen_alloc(0);
    yajl_gen_config(g,yajl_gen_print_callback,&yajl_encode_print,&ctx);
    if (lua_isboolean(L, 2) && lua_toboolean(L, 2)) {
        yajl_gen_config(g,yajl_gen_beautify, 1);
    }
    if (lua_gettop(L)>0) {
        do_json_encode(L,1,g);
    }
    yajl_gen_free(g);
    lua_pushstring(L, ctx.data.c_str());
    return 1;
}

namespace llae {

    void json_gen::map_open(lua::state& l) {
        if (!m_g) l.error("use after free");
        yajl_gen_status status = yajl_gen_map_open(m_g);
        if (status!=yajl_gen_status_ok) {
           l.error("json gen map open failed: %d",status);
        }
    }

    void json_gen::map_close(lua::state& l) {
        if (!m_g) l.error("use after free");
        yajl_gen_status status = yajl_gen_map_close(m_g);
        if (status!=yajl_gen_status_ok) {
           l.error("json gen map close failed: %d",status);
        }
    }

    void json_gen::array_open(lua::state& l) {
        if (!m_g) l.error("use after free");
        yajl_gen_status status = yajl_gen_array_open(m_g);
        if (status!=yajl_gen_status_ok) {
           l.error("json gen array open failed: %d",status);
        }
    }

    void json_gen::array_close(lua::state& l) {
        if (!m_g) l.error("use after free");
        yajl_gen_status status = yajl_gen_array_close(m_g);
        if (status!=yajl_gen_status_ok) {
            l.error("json gen array close failed: %d",status);
        }
    }

    void json_gen::lstring(lua::state& l) {
        if (!m_g) l.error("use after free");
        size_t size = 0;
        const char* name = l.checklstring(2,size);
        yajl_gen_status status = yajl_gen_string(m_g, reinterpret_cast<const unsigned char*>(name), size);
        if (status!=yajl_gen_status_ok) {
            l.error("json gen string failed: %d",status);
        }
    }

    void json_gen::lnull(lua::state& l) {
        if (!m_g) l.error("use after free");
        yajl_gen_status status = yajl_gen_null(m_g);
        if (status!=yajl_gen_status_ok) {
            l.error("json gen null failed: %d",status);
        }
    }

    void json_gen::lbool(lua::state& l) {
        if (!m_g) l.error("use after free");
        int n = l.toboolean(2);
        yajl_gen_status status = yajl_gen_bool(m_g, n);
        if (status!=yajl_gen_status_ok) {
            l.error("json gen bool failed: %d",status);
        }
    }

    void json_gen::linteger(lua::state& l) {
        if (!m_g) l.error("use after free");
        lua_Integer n = l.checkinteger(2);
        yajl_gen_status status = yajl_gen_integer(m_g, n);
        if (status!=yajl_gen_status_ok) {
            l.error("json gen integer failed: %d",status);
        }
    }

    void json_gen::ldouble(lua::state& l) {
        if (!m_g) l.error("use after free");
        auto n = l.checknumber(2);
        yajl_gen_status status = yajl_gen_double(m_g, n);
        if (status!=yajl_gen_status_ok) {
            l.error("json gen double failed: %d",status);
        }
    }

    lua::multiret json_gen::get_buffer(lua::state& l) {
        if (!m_g) l.error("use after free");
        const unsigned char * buf = 0;
        size_t len = 0;
        yajl_gen_status status = yajl_gen_get_buf(m_g,&buf,&len);
        if (status == yajl_gen_generation_complete || status == yajl_gen_status_ok) {
            l.pushlstring(reinterpret_cast<const char*>(buf),len);
            return {1};
        }
        {
            l.error("json gen failed: %d", status);
        }
        return {0};
    }
    uv::buffer_view json_gen::get_data() const {
        if (!m_g) return uv::buffer_view(nullptr,0);
        const unsigned char * buf = 0;
        size_t len = 0;
        yajl_gen_status status = yajl_gen_get_buf(m_g,&buf,&len);
        if (status == yajl_gen_generation_complete || status == yajl_gen_status_ok) {
            return uv::buffer_view(buf,len);
        }
        return uv::buffer_view(nullptr,0);
    }
    void json_gen::free() {
        if (m_g) yajl_gen_free(m_g);
        m_g = nullptr;
    }
    
    json_gen::json_gen() {
        m_g = yajl_gen_alloc(0);
    }
    json_gen::json_gen(json_gen&& o) : m_g(o.m_g) {
        o.m_g = nullptr;
    }
    json_gen::~json_gen() {
        free();
    }
    void json_gen::config(lua::state& l) {
        if (l.isboolean(1)) {
            yajl_gen_config(m_g,yajl_gen_beautify, l.toboolean(1)?1:0);
        }
    }
    lua::multiret json_gen::lnew(lua::state& l) {
        json_gen gen;
        gen.config(l);
        lua::push_raw(l,std::move(gen));
        return {1};
    }
    void json_gen::lbind(lua::state& l) {
        lua::bind::function(l,"new",&json_gen::lnew);
        lua::bind::function(l,"map_open",&json_gen::map_open);
        lua::bind::function(l,"map_close",&json_gen::map_close);
        lua::bind::function(l,"array_open",&json_gen::array_open);
        lua::bind::function(l,"array_close",&json_gen::array_close);
        lua::bind::function(l,"null",&json_gen::lnull);
        lua::bind::function(l,"bool",&json_gen::lbool);
        lua::bind::function(l,"string",&json_gen::lstring);
        lua::bind::function(l,"double",&json_gen::ldouble);
        lua::bind::function(l,"integer",&json_gen::linteger);
        lua::bind::function(l,"free",&json_gen::free);
        lua::bind::function(l,"get_buffer",&json_gen::get_buffer);
    }
};
META_INFO(llae::json_gen,void)

static int json_array(lua_State* L) {
    luaL_checktype(L,1,LUA_TTABLE);
    lua_getfield(L,LUA_REGISTRYINDEX,"json_array");
    lua_setmetatable(L,1);
    lua_pushvalue(L,1);
    return 1;
}

static int is_json_array(lua_State* L) {
    bool is_array = false;
    if (lua_type(L,1) == LUA_TTABLE) {
        if (lua_getmetatable(L,1)) {
            if (lua_getfield(L,-1,"__json_type")==LUA_TLIGHTUSERDATA) {
                is_array = lua_touserdata(L,-1) == &json_array_marker;
            }
            lua_pop(L,2);
        }
    }
    lua_pushboolean(L,is_array ? 1 : 0);
    return 1;
}


int luaopen_json(lua_State* L) {
    lua::state l{L};
    l.newtable();
    lua::bind::object<llae::json_gen>::register_metatable(l,&llae::json_gen::lbind);
    lua::bind::function(l,"encode",&json_encode);
    lua::bind::function(l,"decode",&json_decode);
    lua::bind::function(l,"array",&json_array);
    lua::bind::function(l,"is_array",&is_json_array);
    lua::bind::object<llae::json_gen>::get_metatable(l);
    l.setfield(-2, "gen");
    
    luaL_newmetatable(L,"json_array");
    lua_pushlightuserdata(L,&json_array_marker);
    lua_setfield(L,-2,"__json_type");
    lua_pop(L,1);
   

    return 1;
}

    
