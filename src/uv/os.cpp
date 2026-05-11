#include "os.h"
#include "luv.h"
#include <memory>

namespace lua {
	int stack<uv_utsname_t>::push(state& l,const uv_utsname_t& n) {
		l.createtable(0,4);
		l.pushstring(n.sysname);
		l.setfield(-2,"sysname");
		l.pushstring(n.release);
		l.setfield(-2,"release");
		l.pushstring(n.version);
		l.setfield(-2,"version");
		l.pushstring(n.machine);
		l.setfield(-2,"machine");
		return 1;
	}
	int stack<std::unordered_map<std::string,std::string>>::push(state& l,const std::unordered_map<std::string,std::string>& m) {
		l.newtable();
		for (const auto& [key,value] : m) {
			l.pushstring(key.c_str());
			l.pushstring(value.c_str());
			l.settable(-3);
		}
		return 1;
	}
}

namespace uv {


	llae::result<std::string> os::homedir() {
		size_t size = 1;
		char dummy;
		auto r = uv_os_homedir(&dummy,&size);
		if (r == UV_ENOBUFS) {
            std::unique_ptr<char[]> data(new char[size]);
			r = uv_os_homedir(data.get(),&size);
            if (r>=0) {
				return std::string(data.get(),size);
			}
		}
		return status_error::create(r);
	}
	llae::result<std::string> os::tmpdir() {
		size_t size = 1;
        char dummy;
		auto r = uv_os_tmpdir(&dummy,&size);
		if (r == UV_ENOBUFS) {
            std::unique_ptr<char[]> data(new char[size]);
			r = uv_os_tmpdir(data.get(),&size);
            if (r>=0) {
				return std::string(data.get(),size);
			}
		}
		return status_error::create(r);
	}
	llae::result<std::string> os::getenv(std::string_view name) {
		size_t size = 1;
        char dummy;
		auto r = uv_os_getenv(name.data(),&dummy,&size);
		if (r == UV_ENOBUFS) {
            std::unique_ptr<char[]> data(new char[size]);
			r = uv_os_getenv(name.data(),data.get(),&size);
            if (r>=0) {
				return std::string(data.get(),size);
			}
		}
		return status_error::create(r);
	}
	llae::result<std::unordered_map<std::string,std::string>> os::getallenv() {
		uv_env_item_t* envs = nullptr;
		int count = 0;
		auto r = uv_os_environ(&envs,&count);
		if (r<0) {
			return status_error::create(r);
		}
		std::unordered_map<std::string,std::string> result;
		for (int i=0;i<count;++i) {
			result[envs[i].name] = envs[i].value;
		}
		uv_os_free_environ(envs,count);
		return std::move(result);
	}
	llae::result<> os::setenv(std::string_view name, std::string_view value) {
		auto r = uv_os_setenv(name.data(),value.data());
		if (r<0) {
			return status_error::create(r);
		}
		return llae::result<>();
	}
	llae::result<> os::unsetenv(std::string_view name) {
		auto r = uv_os_unsetenv(name.data());
		if (r<0) {
			return status_error::create(r);
		}
		return llae::result<>();
	}
	llae::result<std::string> os::gethostname() {
        size_t size = 1;
        char dummy;
		auto r = uv_os_gethostname(&dummy,&size);
		if (r == UV_ENOBUFS) {
            std::unique_ptr<char[]> data(new char[size]);
			r = uv_os_gethostname(data.get(),&size);
            if (r>=0) {
				return std::string(data.get(),size);
			}
		}
		return status_error::create(r);
	}
	llae::result<uv_utsname_t> os::uname() {
		uv_utsname_t n;
		auto r = uv_os_uname(&n);
		if (r<0) { return status_error::create(r); }
		return n;
	}

	int os::getpid() {
		return uv_os_getpid();
	}

	llae::result<int> os::getpriority(std::optional<int> pid) {
		int prio = 0;
		auto r = uv_os_getpriority(pid.has_value() ? static_cast<uv_pid_t>(pid.value()) : uv_os_getpid(),&prio);
		if (r<0) { return status_error::create(r); }
		return prio;
	}

	llae::result<> os::setpriority(int priority,std::optional<int> pid) {
		auto r = uv_os_setpriority(pid.has_value() ? static_cast<uv_pid_t>(pid.value()) : uv_os_getpid(),priority);
		if (r<0) { return status_error::create(r); }
		return llae::result<>();
	}


}
