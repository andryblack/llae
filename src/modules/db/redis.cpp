#include "lua/bind.h"
#include "uv/tcp_connection.h"
#include "uv/buffer.h"
#include <cmath>
#include <cstdlib>


static inline size_t number_len(size_t number) {
    return number == 0 ? 1 : (std::log10(number) + 1);
}

static int lua_redis_resp_gen_req(lua_State* L) {
    lua::state l(L);
    int top = l.gettop();
    if (top < 1) {
        l.error("need command");
    }
    if (top > 9999) {
        l.error("too many args");
    }
    auto res = uv::buffer::alloc(1+number_len(top)+2+2+top*16); // euristic
    res->set_len(0);
    auto require_size = [&res](size_t size) {
        while ((res->get_len()+size) > res->get_capacity()) {
            res = res->realloc(res->get_capacity()*3/2);
        }
    }
    auto write_ch = [&res,&require_size](char c) {
        require_size(0);
        static_cast<char*>(res->get_base())[res->get_len()] = c;
        res->set_len(res->get_len()+1);
    };
    auto write_rn = [&res]() {
        require_size(2);
        static_cast<char*>(res->get_base())[res->get_len()] = '\r';
        static_cast<char*>(res->get_base())[res->get_len()+1] = '\n';
        res->set_len(res->get_len()+2);
    };
    auto write_data = [&res](const void* data,size_t size) {
        require_size(size);
        memcpy(static_cast<char*>(res->get_base())+res->get_len(),data,size);
        res->set_len(res->get_len()+size);
    };
    auto write_number = [&res](size_t number) {
        char buf[16];
        ::snprintf(buf,sizeof(buf)-1,"%d",int(number));
        size_t sz = ::strlen(buf);
        require_size(sz);
        memcpy(static_cast<char*>(res->get_base())+res->get_len(),buf,sz);
        res->set_len(res->get_len()+sz);
    };
    write_ch('*');
    write_number(top);
    write_rn();
    for (int i=1;i<=top;++i) {
        auto t = l.get_type(i);
        write_ch('$');
        if (t == lua::value_type::lnil) {
            write_ch('-');
            write_ch('1');
        } else if (t == lua::value_type::string) {
            size_t sz = 0;
            auto str = l.tolstring(i,sz);
            write_number(sz);
            write_rn();
            write_data(str,sz);
        } else if (t == lua::value_type::userdata) {
            auto buf = uv::buffer_base::get(l,i,true);
            if (!buf) {
                l.argerror(i,"invalid type");
            } else {
                write_number(buf->get_len());
                write_rn();
                write_data(buf->get_base(),buf->get_len());
            }
        } else if (t == lua::value_type::number ||
            t == lua::value_type::boolean) {
            l.pushvalue(i);
            size_t sz = 0;
            auto str = l.tolstring(-1,sz);
            write_number(sz);
            write_rn();
            write_data(str,sz);
            l.pop(1);
        } else {
            l.argerror(i,"unexpected type");
        }
        write_rn();
    }
    lua::push(l,std::move(res));
    return 1;
}

int luaopen_db_redis_resp(lua_State* L) {
    lua::state l(L);
    l.createtable();
    lua::bind::function(l,"gen_req",&lua_redis_resp_gen_req);
    return 1;
}

