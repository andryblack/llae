#ifndef __LLAE_UV_POLL_H_INCLUDED__
#define __LLAE_UV_POLL_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "common/intrusive_ptr.h"
#include "posix/fd.h"


namespace uv {

	class loop;

	class poll;
	class poll_consumer : public meta::object {
    public:
        virtual bool on_poll(poll* p,int status,int events) = 0;
        virtual void on_poll_closed(poll* s) {}
    };
    typedef common::intrusive_ptr<poll_consumer> poll_consumer_ptr;
	
	class poll : public handle {
		META_OBJECT
	private:
		uv_poll_t m_poll;
		posix::fd_ptr m_fd;
		poll_consumer_ptr m_consumer;
		static void on_poll_cb(uv_poll_t *handle, int status, int events);
		bool on_poll(int status, int events);
	public:
		virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_poll); }
	protected:
		virtual ~poll() override;
		virtual void on_closed() override;
		
	public:
		explicit poll(uv::loop& loop,int fd );
		explicit poll(uv::loop& loop,posix::fd_ptr && fd );
		static void lbind(lua::state& l);
		static lua::multiret lnew(lua::state& l);

		int start_poll(int events,const poll_consumer_ptr& poll);
		int stop_poll();
		lua::multiret lpoll(lua::state& l, int events);
		lua::multiret lstop(lua::state& l);
	};
	typedef common::intrusive_ptr<poll> poll_ptr;

}

#endif /*__LLAE_UV_POLL_H_INCLUDED__*/
