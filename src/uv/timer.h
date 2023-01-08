#ifndef __LLAE_UV_TIMER_H_INCLUDED__
#define __LLAE_UV_TIMER_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "common/intrusive_ptr.h"
#include <functional>

namespace uv {

	class loop;

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
	};

	class timer_lcb : public timer {
		META_OBJECT
	private:
		lua::ref m_cb;
	protected:
		virtual void on_cb() override;
		virtual void on_closed() override;
		explicit timer_lcb(loop& l) : timer(l) {}
	public:
		
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
		lua::multiret lstart(lua::state& l);
		lua::multiret lstop(lua::state& l);
	};

}

#endif /*__LLAE_UV_TIMER_H_INCLUDED__*/
