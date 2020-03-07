#include "fs.h"
#include "llae/app.h"
#include "common/intrusive_ptr.h"
#include "luv.h"

META_OBJECT_INFO(uv::fs,uv::req)

namespace uv {

	fs::fs(lua::ref&& cont) : m_cont(std::move(cont)) {
		uv_req_set_data(reinterpret_cast<uv_req_t*>(&m_fs),this);
	}
	fs::~fs() {
		uv_fs_req_cleanup(&m_fs);
	}

	fs* fs::get(uv_fs_t* req) {
		return static_cast<fs*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
	}
	void fs::fs_cb(uv_fs_t* req) {
		fs* self = get(req);
		auto& l = llae::app::get(self->m_fs.loop).lua();
		self->m_cont.push(l);
		auto toth = l.tothread(-1);
		int nargs = self->on_cb(toth);
		auto s = toth.resume(l,nargs);
		if (s != lua::status::ok && s != lua::status::yield) {
			llae::app::show_error(toth,s);
		}
		l.pop(1);// thread
		self->m_cont.reset(l);
		self->remove_ref();
	}

	class fs_status : public fs {
	public:
		fs_status(lua::ref&& cont) : fs(std::move(cont)) {}
		int on_cb(lua::state& l) override {
			auto res = uv_fs_get_result(fs_req());
			if (res < 0) {
				l.pushnil();
				uv::push_error(l,res);
				return 2;
			} 
			l.pushboolean(true);
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
		auto path = l.checkstring(1);
		int mode = l.optinteger(2,0755);
		llae::app& app(llae::app::get(l));
		lua::ref cont;
		l.pushthread();
		cont.set(l);
		common::intrusive_ptr<fs> req{new fs_status(std::move(cont))};
		req->add_ref();
		int r = uv_fs_mkdir(app.loop().native(),
			req->fs_req(),path,mode,&fs::fs_cb);
		if (r < 0) {
			req->remove_ref();
			l.pushnil();
			uv::push_error(l,r);
			return 2;
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
		auto path = l.checkstring(1);
		auto new_path = l.checkstring(2);
		auto flags = l.optinteger(3,0);
		llae::app& app(llae::app::get(l));
		lua::ref cont;
		l.pushthread();
		cont.set(l);
		common::intrusive_ptr<fs> req{new fs_status(std::move(cont))};
		int r = uv_fs_copyfile(app.loop().native(),
			req->fs_req(),path,new_path,flags,&fs::fs_cb);
		if (r < 0) {
			req->remove_ref();
			l.pushnil();
			uv::push_error(l,r);
			return 2;
		} 
		req->add_ref();
		l.yield(0);
		return 0;
	}
}