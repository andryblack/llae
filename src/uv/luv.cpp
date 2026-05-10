#include "loop.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/bind.h"
#include "lua/types.h"
#include "meta/object.h"
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

META_OBJECT_INFO(uv::status_error,llae::error)

namespace uv {

	const std::string status_error::category = "uv";
	static char uv_error_buf[1024];

	std::string status_error::to_string() const {
		const char* err = uv_strerror_r(get_code(), uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
		return std::string("[uv]:") + err;
	}
	
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
    	l.pushstring(status_error(e).to_string().c_str());
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

	uv_loop_t* get_native(llae::loop& l) {
		return static_cast<uv::loop&>(l).native();
	}
}

int uv::lexepath(lua_State* L) {
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

lua::multiret uv::lgetaddrinfo(lua::state& l) {
	return getaddrinfo_req::getaddrinfo(l);
}

int uv::lcwd(lua_State* L) {
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

int uv::lgettimeofday(lua_State* L) {
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

int uv::lchdir(lua_State* L) {
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

int uv::linterface_addresses(lua_State* L) {
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
		l.pushinteger(i);
		l.setfield(-2,"index");
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

int uv::lset_process_title(lua_State* L) {
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

int uv::lget_free_memory(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_get_free_memory());
	return 1;
}

int uv::lget_total_memory(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_get_total_memory());
	return 1;
}

int uv::lget_constrained_memory(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_get_constrained_memory());
	return 1;
}

int uv::lget_get_available_memory(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_get_available_memory());
	return 1;
}

int uv::lhrtime(lua_State* L) {
	lua::state l(L);
	l.pushinteger(uv_hrtime());
	return 1;
}

int uv::lsleep(lua_State* L) {
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

int uv::lrandom(lua_State* L) {
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

int uv::lprint_handles(lua_State* L) {
    lua::state l(L);
    bool active = l.toboolean(1);
    llae::app& app(llae::app::get(l));
    uv_walk(app.loop().native(),print_walk_cb,&active);
    return 0;
}

int uv::lcpu_info(lua_State* L) {
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

lua::multiret uv::lip4_addr(lua::state& l) {
	const char* host = l.checkstring(1);
	struct sockaddr_in addr;
	auto r = uv_ip4_addr(host, 0, &addr);
	if (r < 0) {
		return uv::return_status_error(l,r);
	}
	l.pushlstring(reinterpret_cast<const char*>(&addr.sin_addr),4);
	return {1};
}

lua::multiret uv::lip6_addr(lua::state& l) {
	const char* host = l.checkstring(1);
	struct sockaddr_in6 addr;
	auto r = uv_ip6_addr(host, 0, &addr);
	if (r < 0) {
		return uv::return_status_error(l,r);
	}
	l.pushlstring(reinterpret_cast<const char*>(&addr.sin6_addr),16);
	return {1};
}

lua::multiret uv::lip4_name(lua::state& l) {
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

lua::multiret uv::lip6_name(lua::state& l) {
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

lua::multiret uv::lif_indextoname(lua::state& l) {
	int index = l.checkinteger(1);
	char ifname[UV_IF_NAMESIZE];
	size_t size = sizeof(ifname);
	auto r = uv_if_indextoname(index, ifname, &size);
	if (r < 0) {
		return uv::return_status_error(l,r);
	}
	l.pushstring(ifname);
	return {1};
}
