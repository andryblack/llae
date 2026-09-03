#ifndef __LLAE_UV_POLL_H_INCLUDED__
#define __LLAE_UV_POLL_H_INCLUDED__

#include "handle.h"
#include "llae/promise.h"
#include "common/intrusive_ptr.h"
#include "posix/fd.h"


namespace uv {

	class loop;

	class poll;
	typedef common::intrusive_ptr<poll> poll_ptr;
	
	/// @luabind
	class poll : public handle {
		META_OBJECT
	private:
		uv_poll_t m_poll;
		posix::fd_ptr m_fd;
		llae::result_promise_ptr<int> m_poll_promise;
		static void on_poll_cb(uv_poll_t *handle, int status, int events);
		void on_poll(int status, int events);
	public:
		virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_poll); }
	protected:
		virtual ~poll() override;
		virtual void on_closed() override;
		
		void resolve(int status);
		void reject(llae::error_ptr&& error);
	public:
		explicit poll(uv::loop& loop,int fd );
		explicit poll(uv::loop& loop,posix::fd_ptr && fd );
		/// @luabind(name=new)
		static poll_ptr lnew(lua::state& l);

		/// @luabind(async=true,name=poll)
		llae::result_promise_ptr<int> poll_async(int events);
		/// @luabind(name=stop)
		llae::result<void> stop_poll();

		/// @luabind(ltype=integer)
		static constexpr auto READABLE = UV_READABLE;
		/// @luabind(ltype=integer)
		static constexpr auto WRITABLE = UV_WRITABLE;
		/// @luabind(ltype=integer)
		static constexpr auto PRIORITIZED = UV_PRIORITIZED;
		/// @luabind(ltype=integer)
		static constexpr auto DISCONNECT = UV_DISCONNECT;
	};
	

}

#endif /*__LLAE_UV_POLL_H_INCLUDED__*/
