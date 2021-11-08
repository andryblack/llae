#include "process.h"
#include "loop.h"
#include "llae/app.h"
#include "luv.h"
#include "lua/bind.h"

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
		auto r = uv_spawn(l.native(),&m_process,options);
		if (r >= 0) {
			attach();
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
		if (m_on_exit.valid()) {
			if (m_on_exit_cont) {
				m_on_exit.push(l);
                auto toth = l.tothread(-1);
                l.pop(1);// thread
                toth.pushinteger(exit_status);
                toth.pushinteger(term_signal);
                lua::ref ref(std::move(m_on_exit));
                auto s = toth.resume(l,2);
                if (s != lua::status::ok && s != lua::status::yield) {
                    llae::app::show_error(toth,s);
                }
                ref.reset(l);
			} else {
				// todo call
				m_on_exit.reset(l);
			}
		} 
	}

	void process::on_closed() {
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
		l.getfield(1,"on_exit");
		if (l.isfunction(-1)) {
			pr->m_on_exit.set(l);
		} else {
			l.pop(1);
		}
		
		std::vector<std::string> args;
		l.getfield(1,"args");
		if (l.istable(-1)) {
			lua_Integer i = 1;
			while (true) {
				l.seti(-1,i);
				if (l.isnumber(-1) || l.isstring(-1)) {
					args.emplace_back(l.tostring(-1));
				} else {
					break;
				}
				l.pop(1);
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
		}
		l.pop(1);

		std::vector<char*> args_list;
		args_list.reserve(args.size()+1);
		args_list.push_back(const_cast<char*>(options.file));
		for (auto& s:args) {
			args_list.push_back(const_cast<char*>(s.c_str()));
		}
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
		options.flags = 0;
		options.stdio_count = 0;
		options.uid = 0;
		options.gid = 0;

		pr->add_ref();
		auto r = pr->do_spawn(llae::app::get(l).loop(),&options);
		if (r < 0) {
			pr->remove_ref();
			l.pushnil();
			push_error(l,r);
			return {2};
		}


		lua::push(l,std::move(pr));
		return {1};
	}

	lua::multiret process::kill(lua::state& l,int signal) {
		auto res = uv_process_kill(&m_process,signal);
		if (res<0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}


	void process::lbind(lua::state& l) {
		lua::bind::function(l,"spawn",&process::spawn);
		lua::bind::function(l,"kill",&process::kill);		
	}
}