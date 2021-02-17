#include "bignum.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include "uv/buffer.h"
#include <memory>
#include <mbedtls/error.h>

META_OBJECT_INFO(crypto::bignum,meta::object)

namespace crypto {

    
    static void check_error(lua::state& l,int err) {
        if (err != 0) {
            char buffer[128] = {0};
            mbedtls_strerror(err,buffer,sizeof(buffer));
            l.pushstring(buffer);
            l.error();
        }
    }

    bignum::bignum() {
        mbedtls_mpi_init(&m_mpi);
    }

    bignum::~bignum() {
        mbedtls_mpi_free(&m_mpi);
    }

    void bignum::set(mbedtls_mpi_sint val) {
        mbedtls_mpi_lset(&m_mpi, val);
    }
    void bignum::set(const bignum& val) {
        mbedtls_mpi_copy(&m_mpi, &val.m_mpi);
    }

    void bignum::self_add(lua::state& l) {
        if (l.gettop()<2) {
            l.argerror(2, "need value");
        }
        if (l.isnumber(2)) {
            check_error(l,mbedtls_mpi_add_int(&m_mpi, &m_mpi, l.tointeger(2)));
        } else {
            auto ptr = lua::stack<const bignum*>::get(l, 2);
            if (!ptr) l.argerror(2, "need number");
            check_error(l,mbedtls_mpi_add_mpi(&m_mpi, &m_mpi, &ptr->m_mpi));
        }
    }

    bignum_ptr bignum::add(lua::state& l) {
        if (l.gettop()<2) {
            l.argerror(2, "need value");
        }
        bignum_ptr res(new bignum());
        if (l.isnumber(2)) {
            check_error(l,mbedtls_mpi_add_int(&res->m_mpi, &m_mpi, l.tointeger(2)));
        } else {
            auto ptr = lua::stack<const bignum*>::get(l, 2);
            if (!ptr) l.argerror(2, "need number");
            check_error(l,mbedtls_mpi_add_mpi(&res->m_mpi, &m_mpi, &ptr->m_mpi));
        }
        return res;
    }

    void bignum::self_mul(lua::state& l) {
        if (l.gettop()<2) {
            l.argerror(2, "need value");
        }
        if (l.isnumber(2)) {
            check_error(l,mbedtls_mpi_mul_int(&m_mpi, &m_mpi, l.tointeger(2)));
        } else {
            auto ptr = lua::stack<const bignum*>::get(l, 2);
            if (!ptr) l.argerror(2, "need number");
            check_error(l,mbedtls_mpi_mul_mpi(&m_mpi, &m_mpi, &ptr->m_mpi));
        }
    }

    bignum_ptr bignum::mul(lua::state& l) {
        bignum_ptr res(new bignum());
        if (l.gettop()<2) {
            l.argerror(2, "need value");
        }
        if (l.isnumber(2)) {
            check_error(l,mbedtls_mpi_mul_int(&res->m_mpi, &m_mpi, l.tointeger(2)));
        } else {
            auto ptr = lua::stack<const bignum*>::get(l, 2);
            if (!ptr) l.argerror(2, "need number");
            check_error(l,mbedtls_mpi_mul_mpi(&res->m_mpi, &m_mpi, &ptr->m_mpi));
        }
        return res;
    }

    void bignum::self_sub(lua::state& l) {
        if (l.gettop()<2) {
            l.argerror(2, "need value");
        }
        if (l.isnumber(2)) {
            check_error(l,mbedtls_mpi_sub_int(&m_mpi, &m_mpi, l.tointeger(2)));
        } else {
            auto ptr = lua::stack<const bignum*>::get(l, 2);
            if (!ptr) l.argerror(2, "need number");
            check_error(l,mbedtls_mpi_sub_mpi(&m_mpi, &m_mpi, &ptr->m_mpi));
        }
    }

    bignum_ptr bignum::sub(lua::state& l) {
        bignum_ptr res(new bignum());
        if (l.gettop()<2) {
            l.argerror(2, "need value");
        }
        if (l.isnumber(2)) {
            check_error(l,mbedtls_mpi_sub_int(&res->m_mpi, &m_mpi, l.tointeger(2)));
        } else {
            auto ptr = lua::stack<const bignum*>::get(l, 2);
            if (!ptr) l.argerror(2, "need number");
            check_error(l,mbedtls_mpi_sub_mpi(&res->m_mpi, &m_mpi, &ptr->m_mpi));
        }
        return res;
    }

    lua::multiret bignum::tostring(lua::state& l) {
        auto radix = l.optinteger(2, 16);
        char dummy;
        size_t len = 0;
        int r = mbedtls_mpi_write_string(&m_mpi,radix,&dummy,0,&len);
        if (r != MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL)
            check_error(l, r);
        std::unique_ptr<char[]> buffer(new char[len]);
        check_error(l, mbedtls_mpi_write_string(&m_mpi,radix,buffer.get(),len,&len));
        l.pushstring(buffer.get());
        return {1};
    }

    lua::multiret bignum::write(lua::state& l) {
        size_t len = l.optinteger(2,mbedtls_mpi_size(&m_mpi));
        auto buffer = uv::buffer::alloc(len);
        int r = mbedtls_mpi_write_binary(&m_mpi,static_cast<unsigned char*>(buffer->get_base()),len);
        check_error(l,r);
        lua::push(l,std::move(buffer));
        return {1};
    }

    void bignum::lbind(lua::state& l) {
        lua::bind::function(l,"new",&bignum::lnew);
        lua::bind::function(l,"__add",&bignum::add);
        lua::bind::function(l,"__mul",&bignum::mul);
        lua::bind::function(l,"__sub",&bignum::sub);
        lua::bind::function(l,"__tostring",&bignum::tostring);
        lua::bind::function(l,"tostring",&bignum::tostring);
        lua::bind::function(l,"add",&bignum::add);
        lua::bind::function(l,"mul",&bignum::mul);
        lua::bind::function(l,"sub",&bignum::sub);
        lua::bind::function(l,"self_add",&bignum::self_add);
        lua::bind::function(l,"self_mul",&bignum::self_mul);
        lua::bind::function(l,"self_sub",&bignum::self_sub);
        lua::bind::function(l,"write",&bignum::write);
    }

    lua::multiret bignum::lnew(lua::state& l) {
        bignum_ptr ptr(new bignum());
        if (l.gettop()>0) {
            if (l.isnumber(1)) {
                ptr->set(l.tointeger(1));
            } else if (l.isstring(1)) {
                auto radix = l.optinteger(2, 16);
                check_error(l, mbedtls_mpi_read_string(&ptr->m_mpi, radix, l.tostring(1)));
            } else {
                auto optr = lua::stack<const bignum*>::get(l, 1);
                if (!optr) l.argerror(1, "need number");
                else ptr->set(*optr);
            }
        }
        lua::push(l,ptr);
        return {1};
    }
}
