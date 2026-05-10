#include "fs.h"
#include "llae/app.h"
#include "common/intrusive_ptr.h"
#include "llae/promise.h"
#include "luv.h"
#include "llae/write_buffers.h"
#include <vector>

META_OBJECT_INFO(uv::file,meta::object)

namespace lua {

	
	static void push_timespec(lua::state& l,const uv_timespec_t& ts) {
		l.createtable(0,2);
		l.pushinteger(ts.tv_sec);
		l.setfield(-2,"sec");
		l.pushinteger(ts.tv_nsec);
		l.setfield(-2,"nsec");
	}

	int stack<uv_stat_t>::push(lua::state& l,const uv_stat_t& statbuf) {
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

	int stack<uv::dirent_t>::push(lua::state& l,const uv::dirent_t& ent) {
		l.createtable(0,0);
		l.pushstring(ent.name.c_str());
		l.setfield(-2,"name");
		l.pushinteger(ent.type);
		l.setfield(-2,"type");
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
		return 1;
	}

	int stack<std::vector<uv::dirent_t>>::push(lua::state& l,const std::vector<uv::dirent_t>& entries) {
		l.newtable();
		lua_Integer idx = 1;
		for (const auto& entry : entries) {
			stack<uv::dirent_t>::push(l,entry);
			l.seti(-2,idx);
			++idx;
		}
		return 1;
	}
}

namespace uv {

	
	class fs_req : public req {
	private:
		uv_fs_t	m_fs;
	protected:
		static fs_req* get(uv_fs_t* req);
	protected:
		fs_req();
		virtual ~fs_req() override;
		virtual void on_cb() = 0;
		virtual void release() {}
	public:
		uv_fs_t* get() { return &m_fs; }
		static void fs_cb(uv_fs_t* req);
	};

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
		self->release();
		self->remove_ref();
	}


	class fs_none : public fs_req {
	public:
		explicit fs_none() : fs_req() {}
		virtual void on_cb() override final {}
	};

	class fs_file_req : public fs_req {
	private:
		file_ptr m_file;
	protected:
		uv_file get_file() const { return m_file->get(); }
		size_t get_offset() const { return m_file->get_offset(); }
	protected:
		explicit fs_file_req(file_ptr&& file) : fs_req(),m_file(std::move(file)) {}
		virtual ~fs_file_req() override {}
	};
	
	class fs_cont : public fs_req {
		lua::ref m_cont;
	protected:
		virtual int on_cont(lua::state& l) = 0;
        virtual void release() override {
            auto& l = llae::app::get(get()->loop).lua();
			if (l.native()) {
				m_cont.reset(l);
			} else {
				m_cont.release();
			}
        }
	public:
		explicit fs_cont(lua::ref&& cont) : fs_req(),m_cont(std::move(cont)) {}
		virtual ~fs_cont() override {}
		virtual void on_cb() override final {
			auto& l = llae::app::get(get()->loop).lua();
            if (!l.native()) {
                m_cont.release();
                return;
            }
			l.checkstack(2);
			m_cont.push(l);
			m_cont.reset(l);
			auto toth = l.tothread(-1);
			toth.checkstack(3);
			int nargs = on_cont(toth);
			auto s = toth.resume(l,nargs);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
			l.pop(1);// thread
      	}
        void reset(lua::state& l) {
            m_cont.reset(l);
        }
	};
	
	
	class fs_stat : public fs_req {
	private:
		llae::result_promise_ptr<uv_stat_t> m_result;
	public:
		explicit fs_stat(const llae::result_promise_ptr<uv_stat_t>& result) : fs_req(),m_result(result) {}
		virtual void on_cb() override final {
			auto res = uv_fs_get_result(get());
			if (res < 0) {
				m_result->set_result(status_error::create(res));
			} else {
				m_result->set_result(llae::result<uv_stat_t>(*uv_fs_get_statbuf(get())));
			}
		}
	};


	class fs_promise_status : public fs_req {
	private:
		llae::result_promise_ptr<void> m_result;
	public:
		explicit fs_promise_status(const llae::result_promise_ptr<void>& result) : fs_req(),m_result(result) {}
		virtual void on_cb() override final {
			auto res = uv_fs_get_result(get());
			if (res < 0) {
				m_result->set_result(uv::make_result(res));
			} else {
				m_result->set_result(llae::result<>());
			}
		}
	};

	class fs_file_promise_status : public fs_file_req {
	private:
		llae::result_promise_ptr<void> m_result;
	public:
		explicit fs_file_promise_status(file_ptr&& file,const llae::result_promise_ptr<void>& result) : fs_file_req(std::move(file)),m_result(result) {}
		virtual void on_cb() override final {
			auto res = uv_fs_get_result(get());
			if (res < 0) {
				m_result->set_result(uv::make_result(res));
			} else {
				m_result->set_result(llae::result<>());
			}
		}
	};

	class fs_scandir : public fs_req {
	private:
		llae::result_promise_ptr<std::vector<dirent_t>> m_result;
	public:
		explicit fs_scandir(const llae::result_promise_ptr<std::vector<dirent_t>>& result) : fs_req(),m_result(result) {}
		virtual void on_cb() override final {
			auto req = get();
			auto res = uv_fs_get_result(req);
			if (res < 0) {
				m_result->set_result(uv::status_error::create(res));
				return;
			} 
			std::vector<dirent_t> entries;
			while(true) {
				uv_dirent_t ent;
				int r = uv_fs_scandir_next(req,&ent);
				if (r == UV_EOF) {
					break;
				}
				dirent_t rent;
				rent.name = ent.name;
				rent.type = ent.type;
				entries.emplace_back(std::move(rent));
			}
			m_result->set_result(std::move(entries));
		}
	};

	class fs_open : public fs_req {
	private:
		llae::result_promise_ptr<file_ptr> m_result;
	public:
		explicit fs_open(const llae::result_promise_ptr<file_ptr>& result) : fs_req(),m_result(result) {}
		virtual void on_cb() override final {
			auto res = uv_fs_get_result(get());
			if (res < 0) {
				m_result->set_result(uv::status_error::create(res));
			} else {
				auto f = common::make_intrusive<file>(uv_file(res),get()->loop);
				m_result->set_result(std::move(f));
			}
		}
	};
	

	llae::result_promise_ptr<void> fs::async_mkdir(llae::loop& l,std::string_view path,std::optional<int> mode) {
		auto result = common::make_intrusive<llae::result_promise<void>>();
		auto req = common::make_intrusive<fs_promise_status>(result);
		auto res = uv_fs_mkdir(get_native(l),req->get(),path.data(),mode.value_or(0755),&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::make_result(res));
		} else {
			req->add_ref();
		}
		return result;
	}


	llae::result_promise_ptr<void> fs::async_rmdir(llae::loop& l,std::string_view path) {
		auto result = common::make_intrusive<llae::result_promise<void>>();
		auto req = common::make_intrusive<fs_promise_status>(result);
		auto res = uv_fs_rmdir(get_native(l),
				req->get(),path.data(),&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::make_result(res));
		} else {
			req->add_ref();
		}
		return result;
	}

	llae::result_promise_ptr<void> fs::async_unlink(llae::loop& l,std::string_view path) {
		auto result = common::make_intrusive<llae::result_promise<void>>();
		auto req = common::make_intrusive<fs_promise_status>(result);
		auto res = uv_fs_unlink(get_native(l),
			req->get(),path.data(),&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::make_result(res));
		} else {
			req->add_ref();
		}
		return result;
	}

	llae::result_promise_ptr<void> fs::async_copyfile(llae::loop& l,std::string_view path,std::string_view new_path,std::optional<int> flags) {
		auto result = common::make_intrusive<llae::result_promise<void>>();
		auto req = common::make_intrusive<fs_promise_status>(result);
		auto res = uv_fs_copyfile(get_native(l),
			req->get(),path.data(),new_path.data(),flags.value_or(0),&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::make_result(res));
		} else {
			req->add_ref();
		}
		return result;
	}

	llae::result_promise_ptr<void> fs::async_rename(llae::loop& l,std::string_view path,std::string_view new_path) {
		auto result = common::make_intrusive<llae::result_promise<void>>();
		auto req = common::make_intrusive<fs_promise_status>(result);
		auto res = uv_fs_rename(get_native(l),
			req->get(),path.data(),new_path.data(),&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::make_result(res));
		} else {
			req->add_ref();
		}
		return result;
	}

	llae::result_promise_ptr<uv_stat_t> fs::async_stat(llae::loop& l,std::string_view path) {
		auto result = common::make_intrusive<llae::result_promise<uv_stat_t>>();
		auto req = common::make_intrusive<fs_stat>(result);
		auto res = uv_fs_stat(get_native(l),
				req->get(),path.data(),&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::status_error::create(res));
		} else {
			req->add_ref();
		}
		return result;
	}

	llae::result_promise_ptr<std::vector<dirent_t>> fs::async_scandir(llae::loop& l,std::string_view path,std::optional<int> flags) {
		auto result = common::make_intrusive<llae::result_promise<std::vector<dirent_t>>>();
		auto req = common::make_intrusive<fs_scandir>(result);
		auto res = uv_fs_scandir(get_native(l),
			req->get(),path.data(),flags.value_or(0),&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::status_error::create(res));
		} else {
			req->add_ref();
		}
		return result;
	}
		
	llae::result_promise_ptr<file_ptr> fs::async_open(llae::loop& l,std::string_view path,std::optional<int> flags,std::optional<int> mode) {
		auto result = common::make_intrusive<llae::result_promise<file_ptr>>();
		auto req = common::make_intrusive<fs_open>(result);
		auto res = uv_fs_open(get_native(l),
			req->get(),path.data(),flags.value_or(UV_FS_O_RDONLY),mode.value_or(0644),&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::status_error::create(res));
		} else {
			req->add_ref();
		}
		return result;
	}

	llae::result_promise_ptr<void> fs::async_chmod(llae::loop& l,std::string_view path,int mode) {
		auto result = common::make_intrusive<llae::result_promise<void>>();
		auto req = common::make_intrusive<fs_promise_status>(result);
		auto res = uv_fs_chmod(get_native(l),
			req->get(),path.data(),mode,&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::make_result(res));
		} else {
			req->add_ref();
		}
		return result;
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

	
	llae::result_promise_ptr<void> file::async_close(llae::loop& l) {
		auto result = common::make_intrusive<llae::result_promise<void>>();
		if (!m_file) {
			result->set_result(llae::string_error::create("file already closed"));
			return result;
		}
		auto req = common::make_intrusive<fs_file_promise_status>(file_ptr(this),result);
		auto f = m_file;
		m_file = 0;
		int r = uv_fs_close(get_native(l),
			req->get(),f,&fs_req::fs_cb);
		if (r < 0) {
			m_file = f;
			result->set_result(uv::status_error::create(r));
		} else {
			req->add_ref();
		}
		return result;
	}

	llae::result_promise_ptr<void> file::async_fsync(llae::loop& l) {
		auto result = common::make_intrusive<llae::result_promise<void>>();
		if (!m_file) {
			result->set_result(llae::string_error::create("file is closed"));
			return result;
		}
		auto req = common::make_intrusive<fs_file_promise_status>(file_ptr(this),result);
		int r = uv_fs_fsync(get_native(l),
			req->get(),m_file,&fs_req::fs_cb);
		if (r < 0) {
			result->set_result(uv::status_error::create(r));
		} else {
			req->add_ref();
		}
		return result;
	}

	

	class file_write_req : public fs_file_promise_status {
	private:
        llae::buffer_base_ptr m_hold;
	public:
		file_write_req(file_ptr&& file, const llae::result_promise_ptr<void>& result,llae::buffer_base_ptr&& data) : fs_file_promise_status(std::move(file),result),m_hold(std::move(data)) {}
        uv_buf_t get_buffer() {
            return ::uv::get_buffer(m_hold);
        }
	};

	llae::result_promise_ptr<void> file::async_write(llae::loop& l,const llae::buffer_view& data) {
		auto result = common::make_intrusive<llae::result_promise<void>>();
		auto store = llae::buffer::hold(data);
		auto req = common::make_intrusive<file_write_req>(file_ptr(this),result,std::move(store));
		auto buffer = req->get_buffer();
		auto res = uv_fs_write(get_native(l),
			req->get(),m_file,&buffer,1,get_offset(),&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::status_error::create(res));
		} else {
			req->add_ref();
			m_offset += data.get_len();
		}
		return result;
	}

	class fs_write_buffers : public fs_file_promise_status {
	private:
		llae::write_buffers m_buffers;
	public:
		fs_write_buffers(file_ptr&& file,const llae::result_promise_ptr<void>& result,llae::write_buffers&& buffers) : fs_file_promise_status(std::move(file),result),m_buffers(std::move(buffers)) {}
		int start(loop& l) {
			auto buffers = uv::get_buffers(m_buffers.get_buffers());
			return uv_fs_write(l.native(),
				get(),get_file(),buffers.data(),
				static_cast<unsigned int>(buffers.size()),get_offset(),&fs_req::fs_cb);
		}
		virtual void release() override {
			auto& l = llae::app::get(get()->loop).lua();
			if (l.native()) {
				m_buffers.reset(l);
			} else {
				m_buffers.release();
			}
		}
		size_t size() const {
			return m_buffers.get_total_size();
		}
	};

	llae::result_promise_ptr<void> file::lasync_write(lua::state& l) {
		if (!l.isyieldable()) {
			return llae::make_result_promise_string_error<void>("write is async");
		}
		if (!m_file) {
			return llae::make_result_promise_string_error<void>("file is closed");
		}

		llae::result_promise_ptr<void> result = common::make_intrusive<llae::result_promise<void>>();
		
		llae::write_buffers buffers;
		{
			int n = l.gettop();
			for (int i=2;i<=n;++i) {
				l.pushvalue(i);
				if (!buffers.put(l)) {
					buffers.reset(l);
					l.argerror(i,"data expected");
				}
			}
		}
		if (buffers.empty()) {
			result->set_result(llae::result<void>());
			return result;
		}

		auto req = common::make_intrusive<fs_write_buffers>(file_ptr(this),result,std::move(buffers));
		auto res = req->start(llae::app::get(l).loop());
		if (res < 0) {
			result->set_result(uv::status_error::create(res));
		} else {
			m_offset += req->size();
			req->add_ref();
		}
		return result;
	}

    class fs_read : public fs_req {
    private:
        common::intrusive_ptr<file> m_file;
		llae::result_promise_ptr<llae::buffer_ptr> m_result;
		llae::buffer_ptr m_buffer;
    public:
        fs_read(common::intrusive_ptr<file>&& file,const llae::result_promise_ptr<llae::buffer_ptr>& result,llae::buffer_ptr&& buffer) : m_file(std::move(file)),m_result(result),m_buffer(std::move(buffer)) {}
        ~fs_read() {}
        uv_buf_t get_buffer() { return ::uv::get_buffer(m_buffer); }
        virtual void on_cb() override final {
            auto res = uv_fs_get_result(get());
			if (res == UV_EOF) {
				m_buffer.reset();
				m_result->set_result(std::move(m_buffer));
			} else if (res < 0) {
                m_result->set_result(uv::status_error::create(res));
            } else {
				m_buffer->set_len(res);
				m_result->set_result(std::move(m_buffer));
            }
        }
    };

    llae::result_promise_ptr<llae::buffer_ptr> file::async_read(llae::loop& l,std::optional<size_t> size) {
		auto result = common::make_intrusive<llae::result_promise<llae::buffer_ptr>>();
		if (!m_file) {
			result->set_result(llae::string_error::create("file is closed"));
			return result;
		}
		auto buffer = llae::buffer::alloc(size.value_or(1024*16));
		if (!buffer) {
			result->set_result(llae::string_error::create("failed to allocate buffer"));
			return result;
		}
		auto req = common::make_intrusive<fs_read>(file_ptr(this),result,std::move(buffer));
		auto buf = req->get_buffer();
		auto res = uv_fs_read(get_native(l),
                req->get(),m_file,&buf,1,m_offset,&fs_req::fs_cb);
		if (res < 0) {
			result->set_result(uv::status_error::create(res));
		} else {
			m_offset += buf.len;
			req->add_ref();
		}
		return result;
    }
	
}

