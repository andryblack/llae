#include "fs.h"
#include "llae/app.h"
#include "common/intrusive_ptr.h"
#include "luv.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::file,meta::object)

namespace uv {

	

	fs_req::fs_req() {
		uv_req_set_data(reinterpret_cast<uv_req_t*>(&m_fs),this);
	}
	fs_req::~fs_req() {
		uv_fs_req_cleanup(&m_fs);
	}

	fs_req* fs_req::get(uv_fs_t* req) {
		return static_cast<fs_req*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
	}
	void fs_req::fs_cb(uv_fs_t* req) {
		fs_req* self = get(req);
		self->on_cb();
		self->remove_ref();
	}


	class fs_cont : public fs_req {
		lua::ref m_cont;
	protected:
		virtual int on_cont(lua::state& l) = 0;
	public:
		explicit fs_cont(lua::ref&& cont) : m_cont(std::move(cont)) {}
		virtual void on_cb() override final {
			auto& l = llae::app::get(get()->loop).lua();
			l.checkstack(2);
			m_cont.push(l);
			auto toth = l.tothread(-1);
			toth.checkstack(3);
			int nargs = on_cont(toth);
			auto s = toth.resume(l,nargs);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
			l.pop(1);// thread
			m_cont.reset(l);
		}
	};

	class fs_none : public fs_req {
	public:
		explicit fs_none() {}
		virtual void on_cb() override final {}
	};
	class fs_status : public fs_cont {
	public:
		fs_status(lua::ref&& cont) : fs_cont(std::move(cont)) {}
		int on_cont(lua::state& l) override {
			auto res = uv_fs_get_result(get());
			if (res < 0) {
				l.pushnil();
				uv::push_error(l,res);
				return 2;
			} 
			l.pushboolean(true);
			return 1;
		}
	};

	static void push_timespec(lua::state& l,const uv_timespec_t& ts) {
		l.createtable(0,2);
		l.pushinteger(ts.tv_sec);
		l.setfield(-2,"sec");
		l.pushinteger(ts.tv_nsec);
		l.setfield(-2,"nsec");
	}
	
	class fs_stat : public fs_cont {
	public:
		fs_stat(lua::ref&& cont) : fs_cont(std::move(cont)) {}
		int on_cont(lua::state& l) override {
		
			auto res = uv_fs_get_result(get());
			if (res < 0) {
				l.pushnil();
				uv::push_error(l,res);
				return 2;
			} 
			auto& statbuf(*uv_fs_get_statbuf(get()));
			l.createtable(0,0);
			push_timespec(l,statbuf.st_mtim);
			l.setfield(-2,"mtim");
			l.pushinteger(statbuf.st_size);
			l.setfield(-2,"size");
			int fmt = statbuf.st_mode & S_IFMT;
			if (fmt == S_IFREG) {
				l.pushboolean(true);
				l.setfield(-2,"isfile");
			} else if (fmt == S_IFDIR) {
				l.pushboolean(true);
				l.setfield(-2,"isdir");
			}
			return 1;
		}
	};

	class fs_scandir : public fs_cont {
	public:
		fs_scandir(lua::ref&& cont) : fs_cont(std::move(cont)) {}
		int on_cont(lua::state& l) override {
			auto req = get();
			auto res = uv_fs_get_result(req);
			if (res < 0) {
				l.pushnil();
				uv::push_error(l,res);
				return 2;
			} 
			l.createtable();
			lua_Integer idx = 1;
			while(true) {
				uv_dirent_t ent;
				int r = uv_fs_scandir_next(req,&ent);
				if (r == UV_EOF) {
					return 1;
				}
				if (ent.type == UV_DIRENT_FILE || ent.type == UV_DIRENT_DIR) {
					l.createtable();
					l.pushstring(ent.name);
					l.setfield(-2,"name");
					if (ent.type == UV_DIRENT_FILE) {
						l.pushboolean(true);
						l.setfield(-2,"isfile");
					} else if (ent.type == UV_DIRENT_DIR) {
						l.pushboolean(true);
						l.setfield(-2,"isdir");
					} else if (ent.type == UV_DIRENT_LINK) {
						l.pushboolean(true);
						l.setfield(-2,"islink");
					}
					l.seti(-2,idx);
					++idx;
				}
			}
			return 1;
		}
	};

	class fs_open : public fs_cont {
	public:
		fs_open(lua::ref&& cont) : fs_cont(std::move(cont)) {}
		int on_cont(lua::state& l) override {
			auto res = uv_fs_get_result(get());
			if (res < 0) {
				l.pushnil();
				uv::push_error(l,res);
				return 2;
			} 
			common::intrusive_ptr<file> f(new file(res,get()->loop));
			lua::push_meta_object(l,std::move(f));
			return 1;
		}
	};
	

	int fs::mkdir(lua_State* L) {
		lua::state l(L);
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("mkdir is async");
			return 2;
		}
		{
			auto path = l.checkstring(1);
			int mode = l.optinteger(2,0755);
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<fs_req> req{new fs_status(std::move(cont))};
			req->add_ref();
			int r = uv_fs_mkdir(app.loop().native(),
				req->get(),path,mode,&fs_req::fs_cb);
			if (r < 0) {
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return 2;
			} 
		}
		l.yield(0);
		return 0;
	}

	int fs::copyfile(lua_State* L) {
		lua::state l(L);
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("copyfile is async");
			return 2;
		}
		{
			auto path = l.checkstring(1);
			auto new_path = l.checkstring(2);
			auto flags = l.optinteger(3,0);
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<fs_req> req{new fs_status(std::move(cont))};
			req->add_ref();
			int r = uv_fs_copyfile(app.loop().native(),
				req->get(),path,new_path,flags,&fs_req::fs_cb);
			if (r < 0) {
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return 2;
			} 
		}
		l.yield(0);
		return 0;
	}

	int fs::stat(lua_State* L) {
		lua::state l(L);
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("stat is async");
			return 2;
		}
		{
			auto path = l.checkstring(1);
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<fs_req> req{new fs_stat(std::move(cont))};
			req->add_ref();
			int r = uv_fs_stat(app.loop().native(),
				req->get(),path,&fs_req::fs_cb);
			if (r < 0) {
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return 2;
			} 
		}
		l.yield(0);
		return 0;
	}

	int fs::scandir(lua_State* L) {
		lua::state l(L);
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("scandir is async");
			return 2;
		}
		{
			auto path = l.checkstring(1);
			int flags = l.optinteger(2,0);
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<fs_req> req{new fs_scandir(std::move(cont))};
			req->add_ref();
			int r = uv_fs_scandir(app.loop().native(),
				req->get(),path,flags,&fs_req::fs_cb);
			if (r < 0) {
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return 2;
			} 
		}
		l.yield(0);
		return 0;
	}

	int fs::open(lua_State* L) {
		lua::state l(L);
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("open is async");
			return 2;
		}
		{
			auto path = l.checkstring(1);
			int flags = l.optinteger(2,0);
			int mode = l.optinteger(3,UV_FS_O_RDONLY);
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<fs_req> req{new fs_open(std::move(cont))};
			req->add_ref();
			int r = uv_fs_open(app.loop().native(),
				req->get(),path,flags,mode,&fs_req::fs_cb);
			if (r < 0) {
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return 2;
			} 
		}
		l.yield(0);
		return 0;
	}

	file::file(uv_file f,uv_loop_t* l) : m_file(f),m_loop(l) {

	}

	void file::destroy() {
		if (m_file) {
			common::intrusive_ptr<fs_req> req{new fs_none()};
			req->add_ref();
			int r = uv_fs_close(m_loop,
					req->get(),m_file,&fs_req::fs_cb);
			if (r < 0) {
				req->remove_ref();
			}
			m_file = 0;
		}
		meta::object::destroy();
	}

	lua::multiret file::close(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("close is async");
			return {2};
		}
		{
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<fs_req> req{new fs_status(std::move(cont))};
			req->add_ref();
			auto f = m_file;
			m_file = 0;
			int r = uv_fs_close(app.loop().native(),
				req->get(),f,&fs_req::fs_cb);
			if (r < 0) {
				m_file = f;
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}


	void file::lbind(lua::state& l) {
		lua::bind::function(l,"close",&file::close);
	}

	void fs::lbind(lua::state& l) {
		lua::bind::object<uv::file>::register_metatable(l,&uv::file::lbind);
		lua::bind::object<uv::file>::get_metatable(l);
		l.setfield(-2,"file");
		l.createtable();
		lua::bind::function(l,"mkdir",&uv::fs::mkdir);
		lua::bind::function(l,"copyfile",&uv::fs::copyfile);
		lua::bind::function(l,"stat",&uv::fs::stat);
		lua::bind::function(l,"scandir",&uv::fs::scandir);
		lua::bind::function(l,"open",&uv::fs::open);
		l.setfield(-2,"fs");
	}
}
