#include "stream.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include "llae/app.h"
#include "luv.h"

META_OBJECT_INFO(uv::stream,uv::handle)
META_OBJECT_INFO(uv::write_req,uv::req)
META_OBJECT_INFO(uv::shutdown_req,uv::req)

namespace uv {

	write_req::write_req(lua::ref&& cont) : m_cont(std::move(cont)) {
		attach(reinterpret_cast<uv_req_t*>(&m_write));
	}
	write_req::~write_req() {
	}

	void write_req::write_cb(uv_write_t* req, int status) {
		write_req* self = static_cast<write_req*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
		self->on_write(status);
		self->remove_ref();
	}

	void write_req::on_write(int status) {
		auto& l = llae::app::get(m_write.handle->loop).lua();
		for (auto& r:m_refs) {
			r.reset(l);
		}
		if (m_cont.valid()) {
			m_cont.push(l);
			auto toth = l.tothread(-1);
			l.pop(1);// thread
			int args;
			if (status < 0) {
				toth.pushnil();
				uv::push_error(toth,status);
				args = 2;
			} else {
				toth.pushboolean(true);
				args = 1;
			}
			auto s = toth.resume(l,args);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
			m_cont.reset(l);
		}
	}

	void write_req::put(lua::state& l) {
		auto t = l.get_type(-1);
		if (t == lua::value_type::table) {
			size_t tl = l.rawlen(-1);
			m_bufs.reserve(tl);
			m_refs.reserve(tl);
			for (size_t j=0;j<tl;++j) {
				l.rawgeti(-1,j+1);
				size_t size;
				const char* val = l.tolstring(-1,size);
				m_refs.emplace_back();
				m_refs.back().set(l);
				m_bufs.push_back(uv_buf_init(const_cast<char*>(val),size));
			}
			l.pop(1);
		} else {
			size_t size;
			const char* val = l.tolstring(-1,size);
			m_refs.emplace_back();
			m_refs.back().set(l);
			m_bufs.push_back(uv_buf_init(const_cast<char*>(val),size));
		}
	}

	int write_req::write(uv_stream_t* s) {
		return uv_write(&m_write,s,m_bufs.data(),m_bufs.size(),&write_req::write_cb);
	}


	shutdown_req::shutdown_req(lua::ref&& cont) : m_cont(std::move(cont)) {
		attach(reinterpret_cast<uv_req_t*>(&m_shutdown));
	}
	shutdown_req::~shutdown_req() {
	}

	void shutdown_req::shutdown_cb(uv_shutdown_t* req, int status) {
		shutdown_req* self = static_cast<shutdown_req*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
		self->on_shutdown(status);
		self->remove_ref();
	}

	void shutdown_req::on_shutdown(int status) {
		auto& l = llae::app::get(m_shutdown.handle->loop).lua();
		if (m_cont.valid()) {
			m_cont.push(l);
			auto toth = l.tothread(-1);
			l.pop(1);// thread
			int args;
			if (status < 0) {
				toth.pushnil();
				uv::push_error(toth,status);
				args = 2;
			} else {
				toth.pushboolean(true);
				args = 1;
			}
			auto s = toth.resume(l,args);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
			m_cont.reset(l);
		}
	}

	int shutdown_req::shutdown(uv_stream_t* s) {
		return uv_shutdown(&m_shutdown,s,&shutdown_req::shutdown_cb);
	}

	stream::stream() {
	}

	stream::~stream() {
	}

	void stream::on_closed() {
		m_read_cont.reset(llae::app::get(get_stream()->loop).lua());
	}

	void stream::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		buf->base = static_cast<char*>(::malloc(suggested_size));
		buf->len = suggested_size;
	}
	void stream::read_cb(uv_stream_t* s, ssize_t nread, const uv_buf_t* buf) {
		static_cast<stream*>(s->data)->on_read(nread,buf);
		::free(buf->base);
	}

	void stream::on_read(ssize_t nread, const uv_buf_t* buf) {
		auto& l = llae::app::get(get_stream()->loop).lua();
		if (nread != 0) {
			uv_read_stop(get_stream());
		}
		if (m_read_cont.valid()) {
			m_read_cont.push(l);
			auto toth = l.tothread(-1);
			l.pop(1);// thread
			if (nread > 0) {
				toth.pushlstring(buf->base,buf->len);
				toth.pushnil();
			} else if (nread == UV_EOF) {
				toth.pushnil();
				toth.pushnil();
			} else if (nread < 0) {
				toth.pushnil();
				uv::push_error(toth,nread);
			} else { // == 0
				return;
			}
			lua::ref ref(std::move(m_read_cont));
			auto s = toth.resume(l,2);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
			ref.reset(l);
		}
		remove_ref();
	}

	lua::multiret stream::read(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("stream::read is async");
			return {2};
		}
		if (m_read_cont.valid()) {
			l.pushnil();
			l.pushstring("stream::read already read");
			return {2};
		}
		{
			l.pushthread();
			m_read_cont.set(l);

			add_ref();
			int res = uv_read_start(get_stream(), &stream::alloc_cb, &stream::read_cb);
			if (res < 0) {
				l.pushnil();
				uv::push_error(l,res);
				remove_ref();
				return {2};
			}
		}
		l.yield(0);
		return {0};
	}

	lua::multiret stream::write(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("stream::write is async");
			return {2};
		}
		{
			l.pushthread();
			lua::ref write_cont;
			write_cont.set(l);

			common::intrusive_ptr<write_req> req{new write_req(std::move(write_cont))};
		
			l.pushvalue(2);
			req->put(l);

			req->add_ref();
			int r = req->write(get_stream());
			if (r < 0) {
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}

	lua::multiret stream::shutdown(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("stream::shutdown is async");
			return {2};
		}
		
		{
			l.pushthread();
			lua::ref shutdown_cont;
			shutdown_cont.set(l);
		
			common::intrusive_ptr<shutdown_req> req{new shutdown_req(std::move(shutdown_cont))};
		
			req->add_ref();
			int r = req->shutdown(get_stream());
			if (r < 0) {
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			}
		} 
		l.yield(0);
		return {0};
	}

	void stream::close() {
		handle::close();
	}

	void stream::lbind(lua::state& l) {
		lua::bind::function(l,"read",&stream::read);
		lua::bind::function(l,"write",&stream::write);
		lua::bind::function(l,"shutdown",&stream::shutdown);
		lua::bind::function(l,"close",&stream::close);
	}
}