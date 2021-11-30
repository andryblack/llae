#include "process.h"
#include "loop.h"
#include "llae/app.h"
#include "luv.h"
#include "lua/bind.h"
#include "stream.h"

#include <string>
#include <vector>

META_OBJECT_INFO(uv::process,uv::handle)

namespace uv {

	process::process() {
		uv_handle_set_data(get_handle(),nullptr);
	}
	process::~process() {
        
	}

	int process::do_spawn(loop& l,const uv_process_options_t *options) {
        add_ref();
		auto r = uv_spawn(l.native(),&m_process,options);
		if (r >= 0) {
			attach();
        } else {
            remove_ref();
        }
		return r;
	}

	void process::on_exit_cb(uv_process_t* handle, int64_t exit_status, int term_signal) {
		process* self = static_cast<process*>(handle->data);
		self->on_exit(exit_status,term_signal);
		self->remove_ref();
	}

	void process::on_exit(int64_t exit_status, int term_signal) {
		auto& l = llae::app::get(get_handle()->loop).lua();
        m_completed = true;
        m_exit_status = exit_status;
        m_term_signal = term_signal;
        
		if (m_cont.valid()) {
            m_cont.push(l);
            auto toth = l.tothread(-1);
            l.pop(1);// thread
            toth.pushinteger(exit_status);
            toth.pushinteger(term_signal);
            lua::ref ref(std::move(m_cont));
            auto s = toth.resume(l,2);
            if (s != lua::status::ok && s != lua::status::yield) {
                llae::app::show_error(toth,s);
            }
            ref.reset(l);
		} 
	}

	void process::on_closed() {
//        if (uv_handle_get_data(get_handle()) == this) {
//            if (!m_completed) {
//                uv_unref(get_handle());
//            }
//        }
		handle::on_closed();
	}

	lua::multiret process::spawn(lua::state& l) {
		l.checktype(1,lua::value_type::table);
		uv_process_options_t options;
		process_ptr pr{new process()};
		options.exit_cb = &process::on_exit_cb;
		l.getfield(1,"file");
		options.file = l.checkstring(-1);
		l.pop(1);
		
        options.flags = 0;
        l.getfield(1,"flags");
        if (l.isnumber(-1)) {
            options.flags = l.tointeger(-1);
        }
        l.pop(1);
        
		std::vector<std::string> args;
		l.getfield(1,"args");
		if (l.istable(-1)) {
			lua_Integer i = 1;
			while (true) {
				l.geti(-1,i);
				if (l.isnumber(-1) || l.isstring(-1)) {
					args.emplace_back(l.tostring(-1));
				} else {
					break;
				}
				l.pop(1);
                ++i;
			}
			l.pop(1);
		} 
		l.pop(1);

		std::vector<std::string> env;
		l.getfield(1,"env");
		if (l.istable(-1)) {
			l.pushnil();
			while (l.next(-2)) {
				std::pair<std::string,std::string> keyval;
				if (l.isstring(-2)) {
					keyval.first = l.tostring(-2);
				} else {
					l.pushvalue(-2);
					keyval.first = l.tostring(-1);
					l.pop(1);
				}
				keyval.second = l.tostring(-1);
				l.pop(1);
				env.emplace_back(keyval.first+"="+keyval.second);
			}
		} 
		l.pop(1);

		std::string cwd;
		l.getfield(1,"cwd");
		if (l.isstring(-1)) {
			cwd = l.tostring(-1);
        } else {
            cwd = get_cwd();
        }
		l.pop(1);
        
        std::vector<uv_stdio_container_s> streams;
        l.getfield(1,"streams");
        if (l.istable(-1)) {
            lua_Integer i = 1;
            while (true) {
                l.geti(-1,i);
                if (l.istable(-1)) {
                    streams.emplace_back();
                    l.geti(-1, 1);
                    streams.back().flags = static_cast<uv_stdio_flags>(l.tointeger(-1));
                    streams.back().data.fd = 0;
                    streams.back().data.stream = nullptr;
                    
                    if (streams.back().flags & UV_INHERIT_FD) {
                        l.geti(-2, 2);
                        streams.back().data.fd = l.tointeger(-1);
                        l.pop(1);
                    } else if (streams.back().flags & (UV_INHERIT_STREAM | UV_CREATE_PIPE)) {
                        l.geti(-2, 2);
                        auto s = lua::stack<uv::stream_ptr>::get(l,-1);
                        if (!s) {
                            l.argerror(1, "invalid stream");
                        }
                        streams.back().data.stream = s->get_stream();
                        l.pop(1);
                    }
                    l.pop(1);
                } else {
                    break;
                }
                l.pop(1);
                ++i;
            }
            l.pop(1);
        }
        l.pop(1);

		std::vector<char*> args_list;
		args_list.reserve(args.size()+1);
		args_list.push_back(const_cast<char*>(options.file));
		for (auto& s:args) {
			args_list.push_back(const_cast<char*>(s.c_str()));
		}
        args_list.push_back(nullptr);
		options.args = args_list.data();

		std::vector<char*> env_list;
		env_list.reserve(env.size());
		if (!env.empty()) {
			for (auto& kv:env) {
				env_list.push_back(const_cast<char*>(kv.c_str()));
			}
			options.env = env_list.data();
		} else {
			options.env = nullptr;
		}
        options.cwd = cwd.c_str();
		options.stdio_count = streams.size();
        options.stdio = streams.data();
		options.uid = 0;
		options.gid = 0;

		auto r = pr->do_spawn(llae::app::get(l).loop(),&options);
		if (r < 0) {
			l.pushnil();
			push_error(l,r);
			return {2};
		}


		lua::push(l,std::move(pr));
		return {1};
	}

	lua::multiret process::kill(lua::state& l,int signal) {
        if (m_completed) {
            l.pushnil();
            l.pushstring("completed");
            return {2};
        }
		auto res = uv_process_kill(&m_process,signal);
		if (res<0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

    lua::multiret process::wait_exit(lua::state& l) {
        if (m_completed) {
            l.pushinteger(m_exit_status);
            l.pushinteger(m_term_signal);
            return {2};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("process::wait_exit is async");
            return {2};
        }
        {
            l.pushthread();
            m_cont.set(l);
        }
        l.yield(0);
        return {0};
    }


	void process::lbind(lua::state& l) {
		lua::bind::function(l,"spawn",&process::spawn);
		lua::bind::function(l,"kill",&process::kill);
        lua::bind::function(l,"wait_exit",&process::wait_exit);
        
        lua::bind::value(l, "IGNORE", UV_IGNORE);
        lua::bind::value(l, "CREATE_PIPE", UV_CREATE_PIPE);
        lua::bind::value(l, "INHERIT_FD", UV_INHERIT_FD);
        lua::bind::value(l, "INHERIT_STREAM", UV_INHERIT_STREAM);
        lua::bind::value(l, "READABLE_PIPE", UV_READABLE_PIPE);
        lua::bind::value(l, "WRITABLE_PIPE", UV_WRITABLE_PIPE);
        lua::bind::value(l, "NONBLOCK_PIPE", UV_NONBLOCK_PIPE);
        
        lua::bind::value(l, "PROCESS_DETACHED", UV_PROCESS_DETACHED);
	}
}
