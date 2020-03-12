#include "write_file_pipe.h"
#include "llae/app.h"
#include "luv.h"

namespace uv {

	void write_file_pipe::fs_cb(uv_fs_t* req) {
		write_file_pipe* self = static_cast<write_file_pipe*>(req->data);
		self->on_read(uv_fs_get_result(req));
	}
	void write_file_pipe::write_cb(uv_write_t* req, int status) {
		write_file_pipe* self = static_cast<write_file_pipe*>(req->data);
		self->on_write(status);
	}
	void write_file_pipe::on_end(int status) {
		if (m_write_active || m_read_active)
			return;
		if (m_cont.valid()) {
			auto& l = llae::app::get(m_stream->get_stream()->loop).lua();
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
		remove_ref();
	}
	int write_file_pipe::start_write(int size) {
		m_buf.len = size;
		m_write_active = true;
		int r = uv_write(&m_write_req,m_stream->get_stream(),&m_buf,1,&write_file_pipe::write_cb);
		if (r < 0) {
			m_write_active = false;
		} 
		return r;
	}
	
	void write_file_pipe::on_write(int status) {
		m_write_active = false;
		if (status < 0) {
			return on_end(status);
		}
		int r = start_read();
		if (r < 0) 
			return on_end(r);
	}
	int write_file_pipe::start_read() {
		m_read_active = true;
		m_buf.len = BUFFER_SIZE;
		int r = uv_fs_read(m_stream->get_stream()->loop,&m_fs_req,m_file->get(),
			&m_buf,1,m_offset,&fs_cb);
		if (r < 0) {
			m_read_active = false;
		}
		return r;
	}
	void write_file_pipe::on_read(int status) {
		m_read_active = false;
		if (status < 0) {
			return on_end(status);
		}
		if (status > 0) {
			m_offset += status;
			int r = start_write(status);
			if (r < 0) 
				return on_end(r);
		} else {
			return on_end(0);
		}
	}

	write_file_pipe::write_file_pipe(stream_ptr&& s,file_ptr&& f,lua::ref&& c) :
		m_stream(std::move(s)),
		m_file(std::move(f)),
		m_cont(std::move(c)) {
		m_buf = uv_buf_init(m_data_buf,BUFFER_SIZE);
		m_offset = 0;
		m_fs_req.data = this;
		m_write_req.data = this;
	}
	write_file_pipe::~write_file_pipe() {
	}
	int write_file_pipe::start() {
		add_ref();
		int r = start_read();
		if (r < 0) {
			remove_ref();
		}
		return r;
	}
}
