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
#include "pipe_server.h"
#include "async.h"
#include "idle.h"
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
		l.pushinteger(addresses[i].address.address4.sin_family);
		l.setfield(-2,"family");
		buf[0] = 0;
		if (addresses[i].address.address4.sin_family == AF_INET) {
			uv_ip4_name(&addresses[i].address.address4, buf, sizeof(buf));
	    } else {
	    	uv_ip6_name(&addresses[i].address.address6, buf, sizeof(buf));
	    }
	    l.pushstring(buf);
	    l.setfield(-2,"address");
	    buf[0] = 0;
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

static int lua_uv_get_available_memory(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_get_available_memory());
	return 1;
}

static int lua_uv_hrtime(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_hrtime());
	return 1;
}

static int lua_uv_sleep(lua_State* L) {
	lua::state l(L);
	uv_sleep(static_cast<unsigned int>(l.checkinteger(1)));
	return 0;
}

class rand_req : public uv::req {
private:
	uv_random_t	m_random;
	llae::buffer_ptr m_buffer;
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
		m_buffer = llae::buffer::alloc(len);
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

static void print_walk_cb(uv_handle_t* handle,void* arg) {
    bool active = *static_cast<const bool*>(arg);
    if ((active && uv_is_active(handle)) || !active) {
        auto type_name = uv_handle_type_name(uv_handle_get_type(handle));
        if (!type_name) type_name = "unknown";
        std::cout << "handle: " << type_name << " : " <<
        (uv_is_active(handle) ? "active" : "inactive") <<
        (uv_is_closing(handle) ? ",closing" : "") << std::endl;
    }
}

static int lua_uv_print_handles(lua_State* L) {
    lua::state l(L);
    bool active = l.toboolean(1);
    llae::app& app(llae::app::get(l));
    uv_walk(app.loop().native(),print_walk_cb,&active);
    return 0;
}

static int lua_uv_cpu_info(lua_State* L) {
	lua::state l(L);
	uv_cpu_info_t *cpu_infos = nullptr;
	int count = 0;
	auto r = uv_cpu_info(&cpu_infos,&count);
	if (r < 0) {
		l.pushnil();
		uv::push_error(l,r);
		return 2;
	}
	l.createtable(count,0);
	for (int i=0;i<count;++i) {
		l.createtable(0,4);
		l.pushstring(cpu_infos[i].model);
		l.setfield(-2,"model");
		l.pushinteger(cpu_infos[i].speed);
		l.setfield(-2,"speed");
		l.createtable(0,4);
		{
			l.pushinteger(cpu_infos[i].cpu_times.user);
			l.setfield(-2,"user");
			l.pushinteger(cpu_infos[i].cpu_times.nice);
			l.setfield(-2,"nice");
			l.pushinteger(cpu_infos[i].cpu_times.sys);
			l.setfield(-2,"sys");
			l.pushinteger(cpu_infos[i].cpu_times.idle);
			l.setfield(-2,"idle");
		}
		l.setfield(-2,"cpu_times");
	}
	uv_free_cpu_info(cpu_infos,count);
	return 1;
}

int lua_uv_clock_gettime(lua_State* L) {
	lua::state l(L);
	auto clock_id = static_cast<uv_clock_id>(l.checkinteger(1));
	uv_timespec64_t time;
	auto r = uv_clock_gettime(clock_id,&time);
	if (r < 0) {
		l.pushnil();
		uv::push_error(l,r);
		return 2;
	}
	l.pushinteger(time.tv_sec);
	l.pushinteger(time.tv_nsec);
	return 2;
}

static lua::multiret lua_uv_ip4_addr(lua::state& l) {
	const char* host = l.checkstring(1);
	struct sockaddr_in addr;
	auto r = uv_ip4_addr(host, 0, &addr);
	if (r < 0) {
		return uv::return_status_error(l,r);
	}
	l.pushlstring(reinterpret_cast<const char*>(&addr.sin_addr),4);
	return {1};
}

static lua::multiret lua_uv_ip6_addr(lua::state& l) {
	const char* host = l.checkstring(1);
	struct sockaddr_in6 addr;
	auto r = uv_ip6_addr(host, 0, &addr);
	if (r < 0) {
		return uv::return_status_error(l,r);
	}
	l.pushlstring(reinterpret_cast<const char*>(&addr.sin6_addr),16);
	return {1};
}

static lua::multiret lua_uv_ip4_name(lua::state& l) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	size_t len = 0;
	auto raw  = l.checklstring(1,len);
	memcpy(&addr.sin_addr,raw,len);
	char name[INET_ADDRSTRLEN];
	auto r = uv_ip4_name(&addr, name, sizeof(name));
	if (r < 0) {
		return uv::return_status_error(l,r);
	}
	l.pushstring(name);
	return {1};
}

static lua::multiret lua_uv_ip6_name(lua::state& l) {
	struct sockaddr_in6 addr;
	addr.sin6_family = AF_INET6;
	addr.sin6_port = 0;
	size_t len = 0;
	auto raw  = l.checklstring(1,len);
	memcpy(&addr.sin6_addr,raw,len);
	char name[INET6_ADDRSTRLEN];
	auto r = uv_ip6_name(&addr, name, sizeof(name));
	if (r < 0) {
		return uv::return_status_error(l,r);
	}
	l.pushstring(name);
	return {1};
}

int luaopen_uv(lua_State* L) {
	lua::state l(L);

	lua::bind::object<uv::handle>::register_metatable(l);
	lua::bind::object<uv::server>::register_metatable(l,&uv::server::lbind);
	lua::bind::object<uv::tcp_server>::register_metatable(l,&uv::tcp_server::lbind);
	lua::bind::object<uv::stream>::register_metatable(l,&uv::stream::lbind);
	lua::bind::object<uv::tcp_connection>::register_metatable(l,&uv::tcp_connection::lbind);
	lua::bind::object<uv::udp>::register_metatable(l,&uv::udp::lbind);
    lua::bind::object<uv::tty>::register_metatable(l,&uv::tty::lbind);
    lua::bind::object<uv::poll>::register_metatable(l,&uv::poll::lbind);
    lua::bind::object<uv::process>::register_metatable(l,&uv::process::lbind);
    lua::bind::object<uv::pipe>::register_metatable(l,&uv::pipe::lbind);
    lua::bind::object<uv::pipe_server>::register_metatable(l,&uv::pipe_server::lbind);
    lua::bind::object<uv::timer>::register_metatable(l);
    lua::bind::object<uv::timer_lcb>::register_metatable(l,&uv::timer_lcb::lbind);
    lua::bind::object<uv::timer_wait>::register_metatable(l,&uv::timer_wait::lbind);
    lua::bind::object<uv::signal_base>::register_metatable(l);
    lua::bind::object<uv::lua_signal>::register_metatable(l,&uv::lua_signal::lbind);
    lua::bind::object<uv::async>::register_metatable(l);
    lua::bind::object<uv::async_continue>::register_metatable(l);
    lua::bind::object<uv::async_wait>::register_metatable(l,&uv::async_wait::lbind);
    lua::bind::object<uv::idle>::register_metatable(l);
	
	l.createtable();
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
    lua::bind::object<uv::pipe_server>::get_metatable(l);
	l.setfield(-2,"pipe_server");
    lua::bind::object<uv::timer_lcb>::get_metatable(l);
    l.setfield(-2,"timer");
    lua::bind::object<uv::timer_wait>::get_metatable(l);
    l.setfield(-2,"timer_wait");
    lua::bind::object<uv::lua_signal>::get_metatable(l);
    l.setfield(-2,"signal");
    lua::bind::object<uv::async_wait>::get_metatable(l);
    l.setfield(-2,"async");
    
	lua::bind::function(l,"exepath",&lua_uv_exepath);
	lua::bind::function(l,"getaddrinfo",&uv::getaddrinfo_req::getaddrinfo);
	lua::bind::function(l,"cwd",&lua_uv_cwd);
	lua::bind::function(l,"chdir",&lua_uv_chdir);
	lua::bind::function(l,"pause",&uv::timer_pause::pause);
	lua::bind::function(l,"resume_delayed",&uv::timer_pause::resume_delayed);
	lua::bind::function(l,"gettimeofday",&lua_uv_gettimeofday);
	lua::bind::function(l,"interface_addresses",&lua_uv_interface_addresses);
	lua::bind::function(l,"set_process_title",&lua_uv_set_process_title);
	lua::bind::function(l,"get_free_memory",&lua_uv_get_free_memory);
	lua::bind::function(l,"get_total_memory",&lua_uv_get_total_memory);
	lua::bind::function(l,"get_constrained_memory",&lua_uv_get_constrained_memory);
	lua::bind::function(l,"get_get_available_memory",&lua_uv_get_available_memory);
	lua::bind::function(l,"hrtime",&lua_uv_hrtime);
	lua::bind::function(l,"sleep",&lua_uv_sleep);
	lua::bind::function(l,"random",&lua_uv_random);
    lua::bind::function(l,"print_handles", &lua_uv_print_handles);
	lua::bind::function(l,"available_parallelism", &uv_available_parallelism);
	lua::bind::function(l,"cpu_info", &lua_uv_cpu_info);

	lua::bind::function(l,"ip4_addr", &lua_uv_ip4_addr);
	lua::bind::function(l,"ip6_addr", &lua_uv_ip6_addr);
	lua::bind::function(l,"ip4_name", &lua_uv_ip4_name);
	lua::bind::function(l,"ip6_name", &lua_uv_ip6_name);
	
	l.pushinteger(AF_INET);
    l.setfield(-2,"AF_INET");
    l.pushinteger(AF_INET6);
    l.setfield(-2,"AF_INET6");

	l.pushinteger(UV_CLOCK_MONOTONIC);
    l.setfield(-2,"CLOCK_MONOTONIC");
	l.pushinteger(UV_CLOCK_REALTIME);
    l.setfield(-2,"CLOCK_REALTIME");
	
	uv::fs::lbind(l);
	uv::os::lbind(l);
	return 1;
}
