#include "loop.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/bind.h"
#include "tcp_server.h"
#include "stream.h"
#include "tcp_connection.h"
#include "dns.h"
#include "fs.h"
#include "os.h"
#include "udp.h"
#include "tty.h"
#include "poll.h"
#include "process.h"
#include "timer.h"
#include "pipe.h"
#include "llae/app.h"
#include <iostream>
#include <memory>

namespace uv {
	

	static char uv_error_buf[1024];
    void error(lua::state& l,int e) {
        const char* err = uv_strerror_r(e, uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
        l.error("UV error \"%q\"",err);
    }
    void error(int e,const char* file,int line) {
        const char* err = uv_strerror_r(e, uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
        llae::report_diag_error(err,file,line);
    }
    void push_error(lua::state& l,int e) {
        const char* err = uv_strerror_r(e, uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
        l.pushstring(err);
    }
    lua::multiret return_status_error(lua::state& s,int r) {
    	if (r<0) {
    		s.pushnil();
    		push_error(s,r);
    		return {2};
    	}
    	s.pushinteger(r);
    	return {1};
    }
    void print_error(int e) {
        const char* err = uv_strerror_r(e, uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
        std::cout << "ERR: " << err << std::endl;
    }
    std::string get_error(int r) {
    	char uv_error_buf[1024];
    	const char* err = uv_strerror_r(r, uv_error_buf, sizeof(uv_error_buf));
    	if (err) return err;
    	return "unknown";
    }
}

static int lua_uv_exepath(lua_State* L) {
	char path[PATH_MAX];
	lua::state l(L);
	size_t size = sizeof(path);
	auto r = uv_exepath(path, &size);
	if (r<0) {
		l.pushnil();
		uv::push_error(l,r);
		return 2;
	}
	l.pushstring(path);
	return 1;
}

static int lua_uv_cwd(lua_State* L) {
	lua::state l(L);
	size_t size = 1;
	char dummy;
	auto r = uv_cwd(&dummy,&size);
	if (r == UV_ENOBUFS) {
        std::unique_ptr<char[]> data(new char[size]);
		r = uv_cwd(data.get(),&size);
        if (r>=0) {
			lua_pushlstring(L,data.get(),size);
			return 1;
		}
	}
	l.pushnil();
	uv::push_error(l,r);
	return 2;
}

static int lua_uv_gettimeofday(lua_State* L) {
	lua::state l(L);
	uv_timeval64_t tv;
	uv_gettimeofday(&tv);
	l.pushnumber(tv.tv_sec);
	l.pushinteger(tv.tv_usec);
	return 2;
}

std::string uv::get_cwd() {
    size_t size = 1;
    char dummy;
    auto r = uv_cwd(&dummy,&size);
    if (r == UV_ENOBUFS) {
        std::unique_ptr<char[]> data(new char[size]);
        r = uv_cwd(data.get(),&size);
        if (r>=0) {
            return data.get();
        }
    }
    return "";
}

static int lua_uv_chdir(lua_State* L) {
	lua::state l(L);
	const char* dir = l.checkstring(1);
	auto r = uv_chdir(dir);
	if (r>0) {
		l.pushboolean(true);
		return 1;
	}
	l.pushnil();
	uv::push_error(l,r);
	return 2;
}

static int lua_uv_interface_addresses(lua_State* L) {
	lua::state l(L);
	uv_interface_address_t *addresses = nullptr;
	int count = 0;
	auto r = uv_interface_addresses(&addresses,&count);
	if (r < 0) {
		l.pushnil();
		uv::push_error(l,r);
		return 2;
	}
	l.createtable(count,0);
	char buf[64];
	for (int i=0;i<count;++i) {
		l.createtable(0,4);
		l.pushstring(addresses[i].name);
		l.setfield(-2,"name");
		l.pushboolean(addresses[i].is_internal);
		l.setfield(-2,"internal");
		if (addresses[i].address.address4.sin_family == AF_INET) {
	      	uv_ip4_name(&addresses[i].address.address4, buf, sizeof(buf));
	    } else {
	    	uv_ip6_name(&addresses[i].address.address6, buf, sizeof(buf));
	    }
	    l.pushstring(buf);
	    l.setfield(-2,"address");
	    if (addresses[i].netmask.netmask4.sin_family == AF_INET) {
	      	uv_ip4_name(&addresses[i].netmask.netmask4, buf, sizeof(buf));
	    } else {
	    	uv_ip6_name(&addresses[i].netmask.netmask6, buf, sizeof(buf));
	    }
	    l.pushstring(buf);
	    l.setfield(-2,"netmask");
	    l.seti(-2,i+1);
	}
	uv_free_interface_addresses(addresses,count);
	return 1;
}

static int lua_uv_set_process_title(lua_State* L) {
	lua::state l(L);
	const char* name = l.checkstring(1);
	int r = uv_set_process_title(name);
	if (r>0) {
		l.pushboolean(true);
		return 1;
	}
	l.pushnil();
	uv::push_error(l,r);
	return 2;
}

static int lua_uv_get_free_memory(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_get_free_memory());
	return 1;
}

static int lua_uv_get_total_memory(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_get_total_memory());
	return 1;
}

static int lua_uv_get_constrained_memory(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_get_constrained_memory());
	return 1;
}

static int lua_uv_hrtime(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_hrtime());
	return 1;
}

static int lua_uv_sleep(lua_State* L) {
	lua::state l(L);
	uv_sleep(l.checkinteger(1));
	return 0;
}

class rand_req : public uv::req {
private:
	uv_random_t	m_random;
	uv::buffer_ptr m_buffer;
	lua::ref m_cont;
protected:
	static rand_req* get(uv_random_t* req) {
		return static_cast<rand_req*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
	}
protected:
	void on_cb(int status, void *buf, size_t buflen) {
		auto& l = llae::app::get(get()->loop).lua();
        if (!l.native()) {
            release();
            return;
        }
		l.checkstack(2);
		m_cont.push(l);
		auto toth = l.tothread(-1);
		m_cont.reset(l);
		toth.checkstack(3);
		int nargs;
		if (status < 0) {
			toth.pushnil();
			uv::push_error(toth,status);
			nargs = 2;
		} else {
			toth.pushlstring(static_cast<const char*>(buf),buflen);
			nargs = 1;
		}
		auto s = toth.resume(l,nargs);
		if (s != lua::status::ok && s != lua::status::yield) {
			llae::app::show_error(toth,s);
		}
		l.pop(1);// thread
	}
	
public:
	void release() {
        m_cont.release();
    }
    void reset(lua::state& l) {
        m_cont.reset(l);
    }
	uv_random_t* get() { return &m_random; }
	static void on_random_cb(uv_random_t* req,int status, void *buf, size_t buflen) {
		auto self = get(req);
		if (self) {
			self->on_cb(status,buf,buflen);
			self->remove_ref();
		}
	}
	explicit rand_req(lua::ref&& cont) : m_cont(std::move(cont)) {
		uv_req_set_data(reinterpret_cast<uv_req_t*>(&m_random),this);
	}
	~rand_req() {
		uv_req_set_data(reinterpret_cast<uv_req_t*>(&m_random),nullptr);
	}
	int start(uv::loop& loop,size_t len) {
		m_buffer = uv::buffer::alloc(len);
		auto res = uv_random(loop.native(),&m_random,m_buffer->get_base(),len,0,&rand_req::on_random_cb);
		if (res < 0) {

		} else {
			add_ref();
		}
		return res;
	}
};

static int lua_uv_random(lua_State* L) {
	lua::state l(L);
	if (!l.isyieldable()) {
		l.pushnil();
		l.pushstring("uv_random is async");
		return 2;
	}
	size_t size = l.checkinteger(1);
	{
		llae::app& app(llae::app::get(l));
		lua::ref cont;
		l.pushthread();
		cont.set(l);
		common::intrusive_ptr<rand_req> req{new rand_req(std::move(cont))};
		auto r = req->start(app.loop(),size);
		if (r < 0) {
			req->reset(l);
			l.pushnil();
			uv::push_error(l,r);
			return 2;
		} 
	}
	l.yield(0);
	return 0;
}

int luaopen_uv(lua_State* L) {
	lua::state l(L);

	lua::bind::object<uv::handle>::register_metatable(l);
	lua::bind::object<uv::tcp_server>::register_metatable(l,&uv::tcp_server::lbind);
	lua::bind::object<uv::stream>::register_metatable(l,&uv::stream::lbind);
	lua::bind::object<uv::tcp_connection>::register_metatable(l,&uv::tcp_connection::lbind);
	lua::bind::object<uv::buffer_base>::register_metatable(l,&uv::buffer_base::lbind);
	lua::bind::object<uv::buffer>::register_metatable(l,&uv::buffer::lbind);
    lua::bind::object<uv::udp>::register_metatable(l,&uv::udp::lbind);
    lua::bind::object<uv::tty>::register_metatable(l,&uv::tty::lbind);
    lua::bind::object<uv::poll>::register_metatable(l,&uv::poll::lbind);
    lua::bind::object<uv::process>::register_metatable(l,&uv::process::lbind);
    lua::bind::object<uv::pipe>::register_metatable(l,&uv::pipe::lbind);
    lua::bind::object<uv::timer>::register_metatable(l);
    lua::bind::object<uv::timer_lcb>::register_metatable(l,&uv::timer_lcb::lbind);

	l.createtable();
	lua::bind::object<uv::buffer>::get_metatable(l);
	l.setfield(-2,"buffer");
	lua::bind::object<uv::tcp_server>::get_metatable(l);
	l.setfield(-2,"tcp_server");
	lua::bind::object<uv::tcp_connection>::get_metatable(l);
	l.setfield(-2,"tcp_connection");
    lua::bind::object<uv::udp>::get_metatable(l);
    l.setfield(-2,"udp");
    lua::bind::object<uv::tty>::get_metatable(l);
    l.setfield(-2,"tty");
    lua::bind::object<uv::poll>::get_metatable(l);
    l.setfield(-2,"poll");
    lua::bind::object<uv::process>::get_metatable(l);
    l.setfield(-2,"process");
    lua::bind::object<uv::pipe>::get_metatable(l);
    l.setfield(-2,"pipe");
     lua::bind::object<uv::timer_lcb>::get_metatable(l);
    l.setfield(-2,"timer");
    
	lua::bind::function(l,"exepath",&lua_uv_exepath);
	lua::bind::function(l,"getaddrinfo",&uv::getaddrinfo_req::getaddrinfo);
	lua::bind::function(l,"cwd",&lua_uv_cwd);
	lua::bind::function(l,"chdir",&lua_uv_chdir);
	lua::bind::function(l,"pause",&uv::timer_pause::pause);
	lua::bind::function(l,"gettimeofday",&lua_uv_gettimeofday);
	lua::bind::function(l,"interface_addresses",&lua_uv_interface_addresses);
	lua::bind::function(l,"set_process_title",&lua_uv_set_process_title);
	lua::bind::function(l,"get_free_memory",&lua_uv_get_free_memory);
	lua::bind::function(l,"get_total_memory",&lua_uv_get_total_memory);
	lua::bind::function(l,"get_constrained_memory",&lua_uv_get_constrained_memory);
	lua::bind::function(l,"hrtime",&lua_uv_hrtime);
	lua::bind::function(l,"sleep",&lua_uv_sleep);
	lua::bind::function(l,"random",&lua_uv_random);

	uv::fs::lbind(l);
	uv::os::lbind(l);
	return 1;
}
