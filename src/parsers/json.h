#pragma once
#include "lua/state.h"
#include "uv/buffer.h"

typedef struct yajl_gen_t * yajl_gen;

namespace llae {

	class json_build {
	protected:
	    yajl_gen m_g = nullptr;
	    
	public:
	    bool map_open();

	    bool map_close();

	    bool array_open();

	    bool array_close();

	    bool add_string(const std::string_view& val);

	    bool add_null();

	    bool add_bool(bool val);

	    bool add_integer(uint64_t val);

	    bool add_double(double val);
	    
	    void free();

	    uv::buffer_view get_data() const;
	    
	public:
	    explicit json_build();
	    explicit json_build(json_build&& o);
	    ~json_build();
	};
}