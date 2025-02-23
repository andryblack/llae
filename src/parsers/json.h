#pragma once
#include "lua/state.h"
#include "uv/buffer.h"

typedef struct yajl_gen_t * yajl_gen;

namespace llae {

	class json_gen {
	private:
	    yajl_gen m_g = nullptr;
	    
	public:
	    void map_open(lua::state& l);

	    void map_close(lua::state& l);

	    void array_open(lua::state& l);

	    void array_close(lua::state& l);

	    void lstring(lua::state& l);

	    void lnull(lua::state& l);

	    void lbool(lua::state& l);

	    void linteger(lua::state& l);

	    void ldouble(lua::state& l);

	    lua::multiret get_buffer(lua::state& l);
	    
	    void free();

	    uv::buffer_view get_data() const;
	    
	public:
	    explicit json_gen();
	    explicit json_gen(json_gen&& o);
	    ~json_gen();
	    void config(lua::state& l) ;
	    static lua::multiret lnew(lua::state& l);
	    static void lbind(lua::state& l);
	};
}