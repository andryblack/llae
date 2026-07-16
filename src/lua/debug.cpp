#include "debug.h"
#include "headers.h"
#include "llae/logger.h"
#include <cstring>
#include <cstdio>


namespace lua::debug {

    static lua_State* current_state = nullptr;
    static lua_State* main_state = nullptr;

    void set_main_state(lua_State* L) {
        main_state = L;
    }
    void set_current_state(lua_State* L) {
        current_state = L;
    }

    #define LEVELS1	10	/* size of the first part of the stack */
    #define LEVELS2	11	/* size of the second part of the stack */
    
    /*
    ** search for 'objidx' in table at index -1.
    ** return 1 + string at top if find a good name.
    */
    static int findfield (lua_State *L, int objidx, int level) {
        if (level == 0 || !lua_istable(L, -1))
            return 0;  /* not found */
        lua_pushnil(L);  /* start 'next' loop */
        while (lua_next(L, -2)) {  /* for each pair in table */
            if (lua_type(L, -2) == LUA_TSTRING) {  /* ignore non-string keys */
                if (lua_rawequal(L, objidx, -1)) {  /* found object? */
                    lua_pop(L, 1);  /* remove value (but keep name) */
                    return 1;
                }
                else if (findfield(L, objidx, level - 1)) {  /* try recursively */
                    lua_remove(L, -2);  /* remove table (but keep name) */
                    lua_pushliteral(L, ".");
                    lua_insert(L, -2);  /* place '.' between the two names */
                    lua_concat(L, 3);
                    return 1;
                }
            }
            lua_pop(L, 1);  /* remove value */
        }
        return 0;  /* not found */
    }
  
  
    /*
    ** Search for a name for a function in all loaded modules
    */
    static int pushglobalfuncname (lua_State *L, lua_Debug *ar) {
        int top = lua_gettop(L);
        lua_getinfo(L, "f", ar);  /* push function */
        lua_getfield(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        if (findfield(L, top + 1, 2)) {
            const char *name = lua_tostring(L, -1);
            if (strncmp(name, "_G.", 3) == 0) {  /* name start with '_G.'? */
                lua_pushstring(L, name + 3);  /* push name without prefix */
                lua_remove(L, -2);  /* remove original name */
            }
            lua_copy(L, -1, top + 1);  /* move name to proper place */
            lua_pop(L, 2);  /* remove pushed values */
            return 1;
        }
        else {
            lua_settop(L, top);  /* remove function and global table */
            return 0;
        }
    }
  
  
    static void getfuncname (lua_State *L, lua_Debug *ar, char* buffer, size_t size) {
        if (pushglobalfuncname(L, ar)) {  /* try first a global name */
            ::snprintf(buffer, size, "function '%s'", lua_tostring(L, -1));
            lua_remove(L, -2);  /* remove name */
        }
        else if (*ar->namewhat != '\0')  /* is there a name from code? */
        ::snprintf(buffer, size, "%s '%s'", ar->namewhat, ar->name);  /* use it */
        else if (*ar->what == 'm')  /* main? */
            ::snprintf(buffer, size, "main chunk");
        else if (*ar->what != 'C')  /* for Lua functions, use <file:line> */
        ::snprintf(buffer, size, "function <%s:%d>", ar->short_src, ar->linedefined);
        else  /* nothing left... */
        ::snprintf(buffer, size, "?");
    }
  
  
    static int lastlevel (lua_State *L) {
        lua_Debug ar;
        int li = 1, le = 1;
        /* find an upper bound */
        while (lua_getstack(L, le, &ar)) { li = le; le *= 2; }
        /* do a binary search */
        while (li < le) {
            int m = (li + le)/2;
            if (lua_getstack(L, m, &ar)) li = m + 1;
            else le = m;
        }
        return le - 1;
    }

    template <typename Writer>
	static void dump_stack(lua_State* L, lua_State* L1, int level, Writer writer) {
		
       
        lua_Debug ar;
        int last = lastlevel(L1);
        int n1 = (last - level > LEVELS1 + LEVELS2) ? LEVELS1 : -1;

        char buffer[1024];
        char function_buffer[256];
        while (lua_getstack(L1, level++, &ar)) {
            if (n1-- == 0) {  /* too many levels? */
                writer("...");  /* add a '...' */
                level = last - LEVELS2 + 1;  /* and skip to last ones */
            } else {

                lua_getinfo(L1, "Slnt", &ar);
                getfuncname(L,&ar,function_buffer,sizeof(function_buffer));
                if (ar.currentline > 0) {
                    ::snprintf(buffer, sizeof(buffer), "%s:%d: in %s", ar.short_src, ar.currentline, function_buffer);
                } else {
                    ::snprintf(buffer, sizeof(buffer), "%s: in %s", ar.short_src, function_buffer);
                }
                if (ar.istailcall) {
                    ::snprintf(buffer, sizeof(buffer), "(...tail calls...)");
                }
            }
            writer(buffer);
        }
    }


    void print_stack() {
        if (!main_state) {
            LOG_ERROR("no main state");
            return;
        }
        dump_stack(main_state, current_state ? current_state : main_state, 0, [](const char* str) {
            LOG_INFO(str);
        });
    }
}

extern "C"
[[gnu::used, gnu::visibility("default")]]
void debug_print_stack() {
    lua::debug::print_stack();
}


