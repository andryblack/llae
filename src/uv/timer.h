#ifndef __LLAE_UV_TIMER_H_INCLUDED__
#define __LLAE_UV_TIMER_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "lua/ref.h"
#include <functional>

namespace uv {

	class loop;

	class timer : public handle {
		META_OBJECT
	public:
		virtual uv_handle_t* get_handle() override { return reinterpret_cast<uv_handle_t*>(&m_timer); }
	private:
		uv_timer_t m_timer;
		static void timer_cb(uv_timer_t *handle);
	protected:
		virtual void on_cb() = 0;
	public:
		explicit timer(loop& l);
		~timer();
		int stop();
		int start(uint64_t timeout, uint64_t repeat);
	};

	class timer_cb : public timer {
	private:
		std::function<void()> m_cb;
	protected:
		virtual void on_cb() { m_cb(); }
	public:
		explicit timer_cb(loop& l,const std::function<void()>& cb) : timer(l), m_cb(cb) {}
	};

	class timer_pause : public timer {
	private:
		lua::ref m_cont;
	protected:
		virtual void on_cb();
	public:
		explicit timer_pause(lua::state& l);
		static lua::multiret pause(lua::state& l);
	};

}

#endif /*__LLAE_UV_TIMER_H_INCLUDED__*/
