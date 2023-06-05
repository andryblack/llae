#ifndef __LLAE_UV_STREAM_H_INCLUDED__
#define __LLAE_UV_STREAM_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"
#include "lua/ref.h"
#include "req.h"
#include "buffer.h"
#include <vector>

namespace uv {

	class stream;
	typedef common::intrusive_ptr<stream> stream_ptr;

	class write_req : public req {
	private:
		uv_write_t	m_write;
		stream_ptr  m_stream;
	protected:
		uv_loop_t* get_loop() { return m_write.handle->loop; }
		static void write_cb(uv_write_t* req, int status);
		virtual void on_write(int status) = 0;
		int start_write(const uv_buf_t* buffers,size_t count);
	public:
		write_req(stream_ptr&& stream);
		~write_req();
	};
	using write_req_ptr = common::intrusive_ptr<write_req>;

	class write_lua_req : public write_req {
	private:
		lua::ref m_cont;
        write_buffers m_buffers;
    protected:
    	virtual void on_write(int status) override;
    public:
    	write_lua_req(stream_ptr&& stream,lua::ref&& cont);
    	bool putm(lua::state& s,int base);
        bool empty() const { return m_buffers.empty(); }
        void reset(lua::state& s);
		int write();
	};

	class write_buffer_req : public write_req {
	private:
		uv::buffer_base_ptr m_buffer;
	protected:
		virtual void on_write(int status) override;
	public:
		explicit write_buffer_req(stream_ptr&& stream,uv::buffer_base_ptr&& s);
		~write_buffer_req();
		int write();
	};

	class shutdown_req : public req {
	private:
		uv_shutdown_t m_shutdown;
        stream_ptr m_stream;
   protected:
		uv_loop_t* get_loop() { return m_shutdown.handle->loop; }
		static void shutdown_cb(uv_shutdown_t* req, int status);
		virtual void on_shutdown(int status) = 0;
        const stream_ptr& get_stream() const { return m_stream; }
	public:
		shutdown_req(stream_ptr&& stream);
		~shutdown_req();
		int shutdown();
	};

	class shutdown_lua_req : public shutdown_req {
	private:
		lua::ref m_cont;
	protected:
		virtual void on_shutdown(int status) override;
	public:
		void reset(lua::state& l);
		shutdown_lua_req(stream_ptr&& stream,lua::ref&& cont);
	};


    class readable_stream;
    class stream_read_consumer : public meta::object {
    public:
        virtual bool on_read(readable_stream* s,ssize_t nread, buffer_ptr& buffer) = 0;
    };
    typedef common::intrusive_ptr<stream_read_consumer> stream_read_consumer_ptr;

    class readable_stream {
    private:
        stream_read_consumer_ptr m_read_consumer;
        std::vector<uv::buffer_ptr> m_read_buffers;
        class lua_read_consumer;
        common::intrusive_ptr<lua_read_consumer> m_lua_reader;
        bool m_closed = false;
    protected:
        readable_stream();
        ~readable_stream();
        bool is_closed() const { return m_closed; }
        void set_closed() { m_closed = true; }
        bool is_read_active() { return m_read_consumer; }
        void consume_read(ssize_t nread,  buffer_ptr& buffer);
        void on_closed();
        virtual void hold_ref() = 0;
        virtual void unhold_ref() = 0;
        uv::buffer_ptr get_read_buffer(size_t size);
    public:
        virtual int start_read( const stream_read_consumer_ptr& consumer );
        virtual void stop_read();
        lua::multiret read(lua::state& l);
        virtual lua::state& get_lua() = 0;
        void add_read_buffer(uv::buffer_ptr&& b);
    };

	class stream : public handle, public readable_stream {
		META_OBJECT
	private:
	protected:
		explicit stream();
		virtual ~stream() override;
		virtual void on_closed() override;
        virtual void hold_ref() override final { add_ref(); }
        virtual void unhold_ref() override final { remove_ref(); }
	private:
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	protected:
        virtual lua::state& get_lua() override final;
	public:
		virtual uv_stream_t* get_stream() = 0;
		static void lbind(lua::state& l);
		bool write(buffer_base_ptr&& buf);
		lua::multiret write(lua::state& l);
        lua::multiret read(lua::state& l) { return readable_stream::read(l); }
        lua::multiret shutdown(lua::state& l);
		lua::multiret send(lua::state& l);
        void add_read_buffer(uv::buffer_ptr&& b) { readable_stream::add_read_buffer(std::move(b)); }
		void close();
        int start_read( const stream_read_consumer_ptr& consumer ) override;
        void stop_read() override;
        
	};

}

#endif /*__LLAE_UV_STREAM_H_INCLUDED__*/
