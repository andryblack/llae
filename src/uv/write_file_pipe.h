#ifndef __LLAE_UV_WRITE_FILE_PIPE_H_INCLUDED__
#define __LLAE_UV_WRITE_FILE_PIPE_H_INCLUDED__

#include "stream.h"
#include "fs.h"

namespace uv {

	class write_file_pipe : public common::ref_counter_base {
	private:
		stream_ptr  m_stream;
		file_ptr m_file;
		lua::ref m_cont;
		
		int64_t m_offset;

		uv_fs_t	m_fs_req;
		static const size_t BUFFER_SIZE = 1024 * 8;
		char m_data_buf[BUFFER_SIZE];
		uv_buf_t m_buf;

		bool m_write_active = false;
		bool m_read_active = false;
		uv_write_t m_write_req;

		static void fs_cb(uv_fs_t* req);
		static void write_cb(uv_write_t* req, int status);
		void on_end(int status);
		int start_write(int size);
		int start_read();

		void on_read(int status);
		void on_write(int status);
		
	protected:
		~write_file_pipe();
	public:
		write_file_pipe(stream_ptr&& s,file_ptr&& f,lua::ref&& c);
		int start();
	};

}


#endif /*__LLAE_UV_WRITE_FILE_PIPE_H_INCLUDED__*/