#include "fs.h"
#include "llae/app.h"
#include "common/intrusive_ptr.h"
#include "luv.h"
#include "lua/bind.h"
#include <vector>

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
        virtual void release() {
            m_cont.release();
        }
	public:
		explicit fs_cont(lua::ref&& cont) : m_cont(std::move(cont)) {}
		virtual void on_cb() override final {
			auto& l = llae::app::get(get()->loop).lua();
            if (!l.native()) {
                release();
                return;
            }
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
            reset(l);
		}
        void reset(lua::state& l) {
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


	int fs::rmdir(lua_State* L) {
		lua::state l(L);
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("rmdir is async");
			return 2;
		}
		{
			auto path = l.checkstring(1);
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<fs_req> req{new fs_status(std::move(cont))};
			req->add_ref();
			int r = uv_fs_rmdir(app.loop().native(),
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

	int fs::unlink(lua_State* L) {
		lua::state l(L);
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("unlink is async");
			return 2;
		}
		{
			auto path = l.checkstring(1);
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<fs_req> req{new fs_status(std::move(cont))};
			req->add_ref();
			int r = uv_fs_unlink(app.loop().native(),
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
			int flags = l.optinteger(2,UV_FS_O_RDONLY);
			int mode = l.optinteger(3,0644);
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

	class fs_write : public fs_cont {
	private:
		common::intrusive_ptr<file> m_file;
		std::vector<uv_buf_t> m_buffers;
		std::vector<lua::ref> m_data;
		int64_t m_size = 0;
        virtual void release() override {
            fs_cont::release();
            for (auto& r:m_data) {
                r.release();
            }
        }
	public:
		fs_write(common::intrusive_ptr<file>&& file,lua::ref&& cont) : fs_cont(std::move(cont)),m_file(std::move(file)) {}
		std::vector<uv_buf_t>& buffers() { return m_buffers; }
		int64_t size() { return m_size; }
        void reset(lua::state& l) {
            for (auto& r:m_data) {
                r.reset(l);
            }
            m_file.reset();
            fs_cont::reset(l);
        }
		int on_cont(lua::state& l) override {
            reset(l);
			auto res = uv_fs_get_result(get());
			if (res < 0) {
				l.pushnil();
				uv::push_error(l,res);
				return 2;
			} 
			
			l.pushinteger(res);
			return 1;
		}
		void read(lua::state& l) {
			int n = l.gettop();
			for (int i=2;i<=n;++i) {
				size_t s = 0;
				const char* data = l.checklstring(i,s);
				if (s) {
					m_data.emplace_back();
					l.pushvalue(i);
					m_data.back().set(l);
					m_buffers.push_back(uv_buf_init(const_cast<char*>(data),s));
					m_size += s;
				}
			}
		}
	};

	lua::multiret file::write(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("write is async");
			return {2};
		}
		{
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<fs_write> req{new fs_write(common::intrusive_ptr<file>(this),std::move(cont))};
			req->read(l);
			req->add_ref();
			int r = uv_fs_write(app.loop().native(),
				req->get(),m_file,req->buffers().data(),
				req->buffers().size(),m_offset,&fs_req::fs_cb);
			if (r < 0) {
                req->reset(l);
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} else {
				m_offset += req->size();
			}
		}
		l.yield(0);
		return {0};
	}

    class fs_read : public fs_cont {
    private:
        common::intrusive_ptr<file> m_file;
        uv_buf_t m_buffer;
        std::vector<char> m_data;
    public:
        fs_read(common::intrusive_ptr<file>&& file,lua::ref&& cont) : fs_cont(std::move(cont)),m_file(std::move(file)) {}
        ~fs_read() {}
        uv_buf_t& buffer() { return m_buffer; }
        int64_t size() { return m_data.size(); }
        int on_cont(lua::state& l) override {
            auto res = uv_fs_get_result(get());
            if (res < 0) {
                m_file.reset();
                l.pushnil();
                uv::push_error(l,res);
                return 2;
            }
            m_file.reset();
            l.pushlstring(m_data.data(), res);
            return 1;
        }
        void alloc(lua::state& l) {
            m_data.resize(l.checkinteger(2));
            m_buffer = uv_buf_init(m_data.data(), m_data.size());
        }
    };

    lua::multiret file::read(lua::state& l) {
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("write is async");
            return {2};
        }
        {
            llae::app& app(llae::app::get(l));
            lua::ref cont;
            l.pushthread();
            cont.set(l);
            common::intrusive_ptr<fs_read> req{new fs_read(common::intrusive_ptr<file>(this),std::move(cont))};
            req->alloc(l);
            req->add_ref();
            int r = uv_fs_read(app.loop().native(),
                req->get(),m_file,&req->buffer(),1,m_offset,&fs_req::fs_cb);
            if (r < 0) {
                req->remove_ref();
                l.pushnil();
                uv::push_error(l,r);
                return {2};
            } else {
                m_offset += req->size();
            }
        }
        l.yield(0);
        return {0};
    }


	void file::lbind(lua::state& l) {
		lua::bind::function(l,"close",&file::close);
		lua::bind::function(l,"write",&file::write);
        lua::bind::function(l,"read",&file::read);
	}

	void fs::lbind(lua::state& l) {
		lua::bind::object<uv::file>::register_metatable(l,&uv::file::lbind);
		lua::bind::object<uv::file>::get_metatable(l);
		l.setfield(-2,"file");
		l.createtable();
		l.pushinteger(UV_FS_O_RDONLY);
		l.setfield(-2,"O_RDONLY");
		l.pushinteger(UV_FS_O_RDWR);
		l.setfield(-2,"O_RDWR");
		l.pushinteger(UV_FS_O_WRONLY);
		l.setfield(-2,"O_WRONLY");
		l.pushinteger(UV_FS_O_CREAT);
		l.setfield(-2,"O_CREAT");
		l.pushinteger(UV_FS_O_APPEND);
		l.setfield(-2,"O_APPEND");

		
		lua::bind::function(l,"mkdir",&uv::fs::mkdir);
		lua::bind::function(l,"rmdir",&uv::fs::rmdir);
		lua::bind::function(l,"unlink",&uv::fs::unlink);
		lua::bind::function(l,"copyfile",&uv::fs::copyfile);
		lua::bind::function(l,"stat",&uv::fs::stat);
		lua::bind::function(l,"scandir",&uv::fs::scandir);
		lua::bind::function(l,"open",&uv::fs::open);
		l.setfield(-2,"fs");
	}
}
