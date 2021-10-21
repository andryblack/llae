

#include <iostream>
#include <string>

#include "lua/state.h"
#include "lua/embedded.h"
#include "lua/value.h"
#include "uv/loop.h"
#include "uv/handle.h"
#include "llae/app.h"
#include "llae/diag.h"
#include "lua/embedded.h"


int luaopen_uv(lua_State*);
int luaopen_ssl(lua_State*);
int luaopen_llae(lua_State*);
int luaopen_archive(lua_State*);
int luaopen_crypto(lua_State*);
int luaopen_posix(lua_State*);

const lua::embedded_module lua::embedded_module::modules[] = {
	{"uv",&luaopen_uv},
	{"ssl",&luaopen_ssl},
	{"llae",&luaopen_llae},
	{"archive",&luaopen_archive},
	{"crypto",&luaopen_crypto},
	{"posix",&luaopen_posix},
	{nullptr,nullptr}
};

static const char main_code[] = 
"local uv = require 'uv' \n" 
"local utils = require 'llae.utils' \n"

"local commands = require 'commands' \n"

"local cmdargs = utils.parse_args(...) \n"

"if cmdargs.verbose then \n"
"	(require 'llae.log').set_verbose(true) \n"
"end \n"

"local cmdname = cmdargs[1] \n"
"if not cmdname then \n"
" 	cmdname = 'help' \n"
"end \n"

"local cmd = commands.map[ cmdname ] \n"
"if not cmd then \n"
" 	print( 'unknown command', cmdname ) \n"
" 	return \n"
"end \n"
 
"cmd:exec( cmdargs ) \n";


const lua::embedded_script lua::embedded_script::scripts[] = {
	{"_main",reinterpret_cast<const unsigned char*>(main_code),sizeof(main_code)-1,0},
	{nullptr,nullptr,0,0}
};

