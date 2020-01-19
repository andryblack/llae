#ifndef __BIGNUM_H_INCLUDED__
#define __BIGNUM_H_INCLUDED__

#include <openssl/bn.h>
#include "../ref_counter.h"

class Bignum;
typedef Ref<Bignum> BignumRef; 

class Bignum : public RefCounter {
private:
	BIGNUM* m_num;
public:
	explicit Bignum();
	~Bignum();
public:
	void clear(lua_State* L);
	static int import(lua_State* L);
	static int lua_export(lua_State* L);
	static int lua_export_hex(lua_State* L);
	void import_hex(lua_State* L,const char* str);
	void import_i(lua_State* L,int val);
	BignumRef modexp(lua_State* L,const BignumRef& p,const BignumRef& m);
	BignumRef mod(lua_State* L,const BignumRef& m);
	BignumRef add(lua_State* L,const BignumRef& a);
	BignumRef mul(lua_State* L,const BignumRef& a);
	
	static void lbind(lua_State* L);
	static int lnew(lua_State* L);
	void push(lua_State* L);
};


#endif /*__BIGNUM_H_INCLUDED__*/