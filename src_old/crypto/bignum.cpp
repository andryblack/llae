#include "bignum.h"
#include "utils.h"
#include "../luabind.h"
#include <new>

static const char* mt_name = "llae.crypto.Bignum";

Bignum::Bignum() : m_num(BN_new()) {

}

Bignum::~Bignum() {
	BN_free(m_num);
}

void Bignum::clear(lua_State* L) {
	BN_clear(m_num);
}

int Bignum::import(lua_State* L) {
	Bignum* self = BignumRef::get_ptr(L,1);
	size_t len = 0;
	const unsigned char* data = reinterpret_cast<const unsigned char*>(luaL_checklstring(L,2,&len));
	BN_bin2bn(data,len,self->m_num);
	return 0;
}

int Bignum::lua_export(lua_State* L) {
	Bignum* self = BignumRef::get_ptr(L,1);
	luaL_Buffer buf;
	unsigned int size = BN_num_bytes(self->m_num);
    unsigned char* data = reinterpret_cast<unsigned char*>(luaL_buffinitsize(L,&buf,size));
	BN_bn2bin(self->m_num,data);
	luaL_pushresultsize(&buf,size);
	return 1;
}

int Bignum::lua_export_hex(lua_State* L) {
	Bignum* self = BignumRef::get_ptr(L,1);
	char* res = BN_bn2hex(self->m_num);
	lua_pushstring(L,res);
	OPENSSL_free(res);
	return 1;
}

void Bignum::import_hex(lua_State* L,const char* val) {
	int r = BN_hex2bn(&m_num,val);
	if (r == 0) {
		luaL_error(L,"failed Bignum::import");
	}
}

void Bignum::import_i(lua_State* L,int val) {
	int r = BN_set_word(m_num,val);
	if (r == 0) {
		luaL_error(L,"failed Bignum::import_i");
	}
}

BignumRef Bignum::modexp(lua_State* L,const BignumRef& p,const BignumRef& m) {
	BignumRef ref(new Bignum());
	
	BN_CTX* ctx = BN_CTX_new();
	int r = BN_mod_exp(ref->m_num,m_num,p->m_num,m->m_num,ctx);
	BN_CTX_free(ctx);

	if (r == 0) {
		luaL_error(L,"failed Bignum::modexp");
	}
	return ref;
}

BignumRef Bignum::mod(lua_State* L,const BignumRef& m) {
	BignumRef ref(new Bignum());
	
	BN_CTX* ctx = BN_CTX_new();
	int r = BN_mod(ref->m_num,m_num,m->m_num,ctx);
	BN_CTX_free(ctx);

	if (r == 0) {
		luaL_error(L,"failed Bignum::mod");
	}
	return ref;
}
BignumRef Bignum::add(lua_State* L,const BignumRef& a) {
	BignumRef ref(new Bignum());
	
	int r = BN_add(ref->m_num,m_num,a->m_num);
	
	if (r == 0) {
		luaL_error(L,"failed Bignum::add");
	}
	return ref;
}
BignumRef Bignum::mul(lua_State* L,const BignumRef& a) {
	BignumRef ref(new Bignum());
	
	BN_CTX* ctx = BN_CTX_new();
	int r = BN_mul(ref->m_num,m_num,a->m_num,ctx);
	BN_CTX_free(ctx);

	if (r == 0) {
		luaL_error(L,"failed Bignum::mul");
	}
	return ref;
}


int Bignum::lnew(lua_State* L) {
	int nargs = lua_gettop(L);
	BignumRef ref = *
		new (lua_newuserdata(L,sizeof(BignumRef))) 
		BignumRef(new Bignum());
	luaL_setmetatable(L,mt_name);
	if (nargs>0) {
		int t = lua_type(L,1);
		if (t == LUA_TNUMBER) {
			ref->import_i(L,lua_tointeger(L,1));
		} else if (t==LUA_TSTRING) {
			size_t len = 0;
			const unsigned char* data = reinterpret_cast<const unsigned char*>(luaL_checklstring(L,1,&len));
			if (nargs>1 && lua_toboolean(L,2)) {
				BN_hex2bn(&ref->m_num,reinterpret_cast<const char*>(data));
			} else {
				if (BN_bin2bn(data,len,ref->m_num)==0) {
					luaL_error(L,"failed create Bignum from raw data");
				} 
			}
		}
	} 
	return 1;
}

void Bignum::push(lua_State* L) {
	new (lua_newuserdata(L,sizeof(BignumRef))) 
		BignumRef(this);
	luaL_setmetatable(L,mt_name);
}

void Bignum::lbind(lua_State* L) {
	luaL_newmetatable(L,mt_name);
	lua_newtable(L);
	luabind::bind(L,"import",&Bignum::import);
	luabind::bind(L,"export",&Bignum::lua_export);
	luabind::bind(L,"export_hex",&Bignum::lua_export_hex);
	luabind::bind(L,"import_i",&Bignum::import_i);
	luabind::bind(L,"import_hex",&Bignum::import_hex);
	luabind::bind(L,"clear",&Bignum::clear);
	luabind::bind(L,"modexp",&Bignum::modexp);
	luabind::bind(L,"mod",&Bignum::mod);
	luabind::bind(L,"add",&Bignum::add);
	luabind::bind(L,"mul",&Bignum::mul);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&BignumRef::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}