#ifndef __LLAE_ARCHIVE_GZSTREAM_H_INCLUDED__
#define __LLAE_ARCHIVE_GZSTREAM_H_INCLUDED__

#include "meta/object.h"
#include "lua/state.h"
#include "uv/buffer.h"
#include "uv/mutex.h"
#include "uv/stream.h"
#include "common/intrusive_ptr.h"
#include <zlib.h>
#include <vector>

namespace uv {
	class loop;
}

namespace archive {

	class zlibcompress : public meta::object {
		META_OBJECT
	private:
		lua::ref m_write_cont;
		
		class compress_work;
		class compress_buffers;
		class compress_finish;
		class write_file_pipe;
		using compress_work_ptr = common::intrusive_ptr<compress_work>;

		void on_compress_complete(lua::state& l,int status,int z_err);
		void on_finish_compress(lua::state& l);
		void add_compressed(uv::buffer_ptr&& b);
		class async_resume_read;
		common::intrusive_ptr<async_resume_read> m_read_resume;
	protected:
		z_stream m_z;
		
		std::vector<uv::buffer_ptr> m_compressed;
		uv::mutex m_mutex;
		bool m_is_error = false;
		bool m_finished = false;
		virtual void continue_read(lua::state& l) = 0;
	public:
		zlibcompress();
		~zlibcompress();
		int init(uv::loop& l,int level,int method,int windowBits,int memLevel,int strategy);
		bool is_error() const { return m_is_error; }
		lua::multiret write(lua::state& l);
		lua::multiret finish(lua::state& l);
		lua::multiret send(lua::state& l);
		
		bool init_deflate(lua::state& l,int argbase);
		static void lbind(lua::state& l);
	};
	using zlibcompress_ptr = common::intrusive_ptr<zlibcompress>; 

	class zlibcompress_read : public zlibcompress {
		META_OBJECT
	private:
		lua::ref m_read_cont;
		bool m_read_buffer = false;
	protected:
		virtual void continue_read(lua::state& l) override;
	public:
		zlibcompress_read();
		lua::multiret read(lua::state& l);
		lua::multiret read_buffer(lua::state& l);
		static void lbind(lua::state& l);
		static lua::multiret new_deflate(lua::state& l);
	};
	using zlibcompress_read_ptr = common::intrusive_ptr<zlibcompress_read>; 

	class zlibcompress_to_stream;
	using zlibcompress_to_stream_ptr = common::intrusive_ptr<zlibcompress_to_stream>; 
	class zlibcompress_to_stream : public zlibcompress {
		META_OBJECT
	private:
		uv::stream_ptr m_stream;
        bool m_need_shutdown = false;
		class write_buffer_req : public uv::write_buffer_req {
			zlibcompress_to_stream_ptr m_self;
		public:
			write_buffer_req(zlibcompress_to_stream_ptr&& self,uv::buffer_ptr&& b);
			virtual void on_write(int status) override;
		};
		class shutdown_req : public uv::shutdown_req {
			zlibcompress_to_stream_ptr m_self;
		public:
			shutdown_req(zlibcompress_to_stream_ptr&& self);
			virtual void on_shutdown(int status) override;
		};
	protected:
		virtual void continue_read(lua::state& l) override;
	public:
		explicit zlibcompress_to_stream( uv::stream_ptr&& stream );
        lua::multiret shutdown(lua::state& l);
		static void lbind(lua::state& l);
		static lua::multiret new_deflate(lua::state& l);
	};
	
}

#endif /*__LLAE_ARCHIVE_GZSTREAM_H_INCLUDED__*/
