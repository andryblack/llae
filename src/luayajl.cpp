
extern "C" {
#include <yajl_parse.h>
#include <yajl_gen.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <vector>
#include <cassert>
#include <string>
    
struct parse_context {
    lua_State* L;
    int stack_depth;
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
                lua_pushinteger(L, state.back().count);
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
                assert(lua_istable(L, -3));
                assert(lua_isstring(L, -2));
                lua_settable(L, -3);
                stack_depth-=2;
            } else if (state.back().type == state_t::array) {
                assert(lua_istable(L, -3));
                assert(lua_isnumber(L, -2));
                lua_settable(L, -3);
                stack_depth-=2;
            }
        }
        
    }
};
    
static int yajl_parse_null(void * ctx) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    lua_pushnil(c->L);
    ++c->stack_depth;
    c->on_value();
    return 1;
}
static int yajl_parse_boolean(void * ctx, int boolVal) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    lua_pushboolean(c->L, boolVal);
    ++c->stack_depth;
    c->on_value();
    return 1;
}
static int yajl_parse_integer(void * ctx, long long integerVal) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    lua_pushinteger(c->L, integerVal);
    ++c->stack_depth;
    c->on_value();
    return 1;
}
static int yajl_parse_double(void * ctx, double doubleVal) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    lua_pushnumber(c->L, doubleVal);
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
    lua_pushlstring(c->L, reinterpret_cast<const char*>(stringVal),stringLen);
    ++c->stack_depth;
    c->on_value();
    return 1;
}

static int yajl_parse_start_map(void * ctx) {
    parse_context* c = static_cast<parse_context*>(ctx);
    c->on_pre_value();
    lua_checkstack(c->L,5);
    lua_newtable(c->L);
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
    lua_pushlstring(c->L, reinterpret_cast<const char*>(key),stringLen);
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
    lua_checkstack(c->L,5);
    c->on_pre_value();
    lua_newtable(c->L);
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
    
static int json_parse(lua_State* L) {
    luaL_checktype(L,1,LUA_TSTRING);
    size_t len = 0;
    const char* text = lua_tolstring(L, 1, &len);
    yajl_callbacks cb;
    fill_parse_callbacks(cb);
    parse_context ctx;
    ctx.L = L;
    ctx.stack_depth = 0;
    
    yajl_handle h = yajl_alloc(&cb, 0, &ctx);
    yajl_config(h,yajl_allow_comments,1);
    yajl_status s = yajl_parse(h,reinterpret_cast<const unsigned char * >(text),len);
    if (s == yajl_status_ok) {
        s = yajl_complete_parse(h);
    }
    if (s!=yajl_status_ok) {
        unsigned char* err_text = yajl_get_error(h, 1,reinterpret_cast<const unsigned char * >(text),len);
        lua_pop(L, ctx.stack_depth);
        lua_pushstring(L, reinterpret_cast<const char*>(err_text));
        yajl_free_error(h,err_text);
        yajl_free(h);
        lua_error(L);
        return 1;
    }
    yajl_free(h);
    return ctx.stack_depth;
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
    bool only_integers = true;
    lua_checkstack(L,10);
    lua_pushvalue(L, indx);
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
    if (only_integers && count!=0) {
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
    


static int json_gen_free(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    yajl_gen_free(g);
    return 0;
}


static int json_gen_new(lua_State* L) {
    lua_pushlightuserdata(L,yajl_gen_alloc(0));
    luaL_getmetatable(L,"json_gen");
    lua_setmetatable(L,-2);
    return 1;
}

static int json_gen_map_open(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    yajl_gen_status status = yajl_gen_map_open(g);
    if (status!=yajl_gen_status_ok) {
       luaL_error(L,"json gen map open failed: %d",status);
    }
    return 0;
}

static int json_gen_map_close(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    yajl_gen_status status = yajl_gen_map_close(g);
    if (status!=yajl_gen_status_ok) {
       luaL_error(L,"json gen map close failed: %d",status);
    }
    return 0;
}

static int json_gen_array_open(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    yajl_gen_status status = yajl_gen_array_open(g);
    if (status!=yajl_gen_status_ok) {
       luaL_error(L,"json gen array open failed: %d",status);
    }
    return 0;
}

static int json_gen_array_close(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    yajl_gen_status status = yajl_gen_array_close(g);
    if (status!=yajl_gen_status_ok) {
        luaL_error(L,"json gen array close failed: %d",status);
    }
    return 0;
}

static int json_gen_string(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    size_t size = 0;
    const char* name = luaL_checklstring(L,2,&size);
    yajl_gen_status status = yajl_gen_string(g, reinterpret_cast<const unsigned char*>(name), size);
    if (status!=yajl_gen_status_ok) {
        luaL_error(L,"json gen string failed: %d",status);
    }
    return 0;
}

static int json_gen_null(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    yajl_gen_status status = yajl_gen_null(g);
    if (status!=yajl_gen_status_ok) {
        luaL_error(L,"json gen null failed: %d",status);
    }
    return 0;
}

static int json_gen_bool(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    int n = lua_toboolean(L,2);
    yajl_gen_status status = yajl_gen_bool(g, n);
    if (status!=yajl_gen_status_ok) {
        luaL_error(L,"json gen bool failed: %d",status);
    }
    return 0;
}

static int json_gen_integer(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    lua_Integer n = luaL_checkinteger(L,2);
    yajl_gen_status status = yajl_gen_integer(g, n);
    if (status!=yajl_gen_status_ok) {
        luaL_error(L,"json gen integer failed: %d",status);
    }
    return 0;
}

static int json_gen_double(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    lua_Number n = luaL_checknumber (L,2);
    yajl_gen_status status = yajl_gen_double(g, n);
    if (status!=yajl_gen_status_ok) {
        luaL_error(L,"json gen double failed: %d",status);
    }
    return 0;
}

static int json_gen_get_buffer(lua_State* L) {
    yajl_gen g = (yajl_gen)luaL_checkudata(L,1,"json_gen");
    const unsigned char * buf = 0;
    size_t len = 0;
    yajl_gen_status status = yajl_gen_get_buf(g,&buf,&len);
    if (status == yajl_gen_generation_complete || status == yajl_gen_status_ok) {
        lua_pushlstring(L,reinterpret_cast<const char*>(buf),len);
        return 1;
    }
    return luaL_error(L,"json gen failed: %d",status);
}

extern "C" int luaopen_llae_json(lua_State* L) {

    luaL_Reg reg[] = {
        { "encode", json_encode },
        { "decode", json_parse },
        { NULL, NULL }
    };
    /* cjson module table */
    lua_newtable(L);
    luaL_setfuncs(L, reg, 0);

    luaL_newmetatable(L,"json_gen");
    luaL_Reg gen[] = {
        { "new", json_gen_new },
        { "free", json_gen_free },
        { "map_open",   json_gen_map_open },
        { "map_close",   json_gen_map_close },
        { "array_open",   json_gen_array_open },
        { "array_close",   json_gen_array_close },
        { "null",       json_gen_null },
        { "bool",       json_gen_bool },
        { "string",     json_gen_string },
        { "double",     json_gen_double },
        { "integer",    json_gen_integer },
        { "get_buffer", json_gen_get_buffer},
        { NULL, NULL }
    };
    luaL_setfuncs(L, gen, 0);
    lua_pushvalue(L,-1);
    lua_setfield(L,-2,"__index");
    lua_setfield(L,-2,"gen");
    return 1;
}

    
