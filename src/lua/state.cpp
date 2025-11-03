#include "lua/state.h"
#include "lua/value.h"
#include <memory>
#include <cstdlib>

namespace lua {

	std::string get_error_message(state& l,status e) {
		std::string message;
		switch(e) {
            case lua::status::yield:
                message.append("YIELD:\t");
                break;
            case lua::status::errun:
                message.append("ERRRUN:\t");
                break;
            case lua::status::errsyntax:
                message.append("ERRSYNTAX:\t");
                break;
            case lua::status::errmem:
                message.append("ERRMEM:\t");
                break;
            case lua::status::errgcmm:
                message.append("ERRGCMM:\t");
                break;
            case lua::status::errerr:
                message.append("ERRERR:\t");
                break;
            default:
                message.append("UNKNOWN:\t");
                break;
        };
        value err(l,-1);
        if (err.is_string()) {
            message.append(err.tostring());
        } else {
            message.append("unknown");
        }
		return message;
	}

	status state::dostring(const char* str) {
		int r = luaL_dostring(m_L,str);
		return static_cast<status>(r);
	}
	void state::require(const char* name,lua_CFunction openfunc) {
		luaL_requiref(m_L,name,openfunc,0);
		lua_pop(m_L, 1);  /* remove lib */
	}

	main_state::main_state() : state( 0 ) {
		m_L = lua_newstate( &main_state::lua_alloc, this );
	}

	main_state::~main_state() {
		close();
	}


	

	static void getfuncname(lua_Debug* ar,std::string& line) {
        if (*ar->namewhat != '\0')  { /* is there a name from code? */
            line.append(ar->namewhat);
            line.append(" '");
            line.append(ar->name);
            line.append("'");
        } else if (*ar->what == 'm')  { /* main? */
            line.append("main chunk");
        } else if (*ar->what != 'C')  { /* for Lua functions, use <file:line> */
            line.append("function <");
            line.append(ar->short_src);
            line.append(":");
            line.append(std::to_string(ar->linedefined));
            line.append(">");
        } else  { /* nothing left... */
            line.append("?");
        }
    }

	std::vector<std::string> state::get_backtrace() const {
        std::vector<std::string> backtrace;
        luaL_checkstack(m_L, 10, NULL);
        lua_Debug ar;
        int level = 1;
        //int n1 = 10;
        while (lua_getstack(m_L, level++, &ar)) {
            lua_getinfo(m_L, "Slnt", &ar);
            std::string line = ar.short_src;
            line.append(":");
            if (ar.currentline > 0) {
                line.append(std::to_string(ar.currentline));
                line.append(":");
            }
            line.append(" in ");
            getfuncname( &ar,line);
            backtrace.emplace_back(std::move(line));
            if (ar.istailcall) {
                backtrace.emplace_back("(...tail call...)");
            }
        }
		return backtrace;
    }


	void *main_state::lua_alloc (void *ud, void *ptr, size_t osize,
	                                                size_t nsize) {
		if (nsize == 0) {
			::free(ptr);
			return 0;
		}
		if (!ptr || !osize) {
			return ::malloc(nsize);
		}
		return ::realloc(ptr,nsize);

	}

	void main_state::open_libs() {
		luaL_openlibs(m_L);
	}

	void main_state::close() {
		if (m_L) {
			lua_close(m_L);
			m_L = 0;
		}
	}

}