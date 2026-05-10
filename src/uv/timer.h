#ifndef __LLAE_UV_TIMER_H_INCLUDED__
#define __LLAE_UV_TIMER_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "common/intrusive_ptr.h"
#include <functional>
#include "llae/result.h"

namespace llae {
	class loop;
}

namespace uv {

	class loop;

	/// @luabind(hidden=true)
	class timer : public handle {
		META_OBJECT
	public:
		virtual uv_handle_t* get_handle() override { return reinterpret_cast<uv_handle_t*>(&m_timer); }
	private:
		uv_timer_t m_timer;
		bool m_start_ref = false;
		static void timer_cb(uv_timer_t *handle);
		void unref();
	protected:
		virtual void on_cb() = 0;
	public:
		explicit timer(loop& l);
		~timer();
		int stop();
		int start(uint64_t timeout, uint64_t repeat);
	};
	using timer_ptr = common::intrusive_ptr<timer>;

	class timer_cb : public timer {
	private:
		std::function<void()> m_cb;
	protected:
		virtual void on_cb() override { m_cb(); }
	public:
		explicit timer_cb(loop& l,const std::function<void()>& cb) : timer(l), m_cb(cb) {}
	};

	class timer_pause : public timer {
	private:
		lua::ref m_cont;
	protected:
		virtual void on_cb() override;
	public:
		explicit timer_pause(lua::state& l);
		static lua::multiret pause(lua::state& l);
		static lua::multiret resume_delayed(lua::state& l);
	};

	/// @luabind(name=timer)
	class timer_lcb : public timer {
		META_OBJECT
	private:
		lua::ref m_cb;
	protected:
		virtual void on_cb() override;
		virtual void on_closed() override;
		explicit timer_lcb(loop& l) : timer(l) {}
	public:
		
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
		/// @luabind(name=start)
		lua::multiret lstart(lua::state& l);
		/// @luabind(name=stop)
		lua::multiret lstop(lua::state& l);
	};

	/// @luabind
	class timer_wait : public timer {
		META_OBJECT
	private:
		lua::ref m_cont;
		bool m_ready = false;
		bool m_started = false;
	protected:
		virtual void on_cb() override;
		virtual void on_closed() override;
		explicit timer_wait(loop& l) : timer(l) {}
		void resume(lua::state& l,const char* status);
	public:
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
		/// @lparam(timeout,integer)
		/// @lparam(repeat,integer?
		/// @luabind(name=start)
		llae::result<> lstart(lua::state& l,int timeout);
		/// @luabind(name=stop)
		llae::result<> lstop(lua::state& l);
		/// @luabind(name=wait)
		lua::multiret lwait(lua::state& l);
	};

}

#endif /*__LLAE_UV_TIMER_H_INCLUDED__*/
