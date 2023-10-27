#ifndef __LLAE_UV_SIGNAL_H_INCLUDED__
#define __LLAE_UV_SIGNAL_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "lua/ref.h"
#include <functional>

namespace uv {

	class loop;

	class signal_base : public handle {
		META_OBJECT
	public:
		virtual uv_handle_t* get_handle() override { return reinterpret_cast<uv_handle_t*>(&m_sig); }
		~signal_base();
	private:
		uv_signal_t m_sig;
		static void signal_cb(uv_signal_t* sig, int signum);
	protected:
		uv_signal_t& get_signal() { return m_sig; }
		virtual void on_signal( int signum ) = 0;
		explicit signal_base(loop& l);
		int start(int signum);
		int start_oneshot(int signum);
	};

	class signal : public signal_base {
		META_OBJECT
	private:
		std::function<void(int)> m_cb;
	protected:
		virtual void on_signal(int s) override;
	public:
		explicit signal(loop& l) : signal_base(l) {}
		int start(int signum,std::function<void(int)>&& f);
		int start_oneshot(int signum,std::function<void(int)>&& f);
	};

	class lua_signal : public signal_base {
		META_OBJECT
	private:
		lua::ref m_ref;
	protected:
		explicit lua_signal(loop& l) : signal_base(l) {}
		void on_signal(int s) override;
		lua::multiret start_oneshot(lua::state& l, int signum, int i);
		static lua::multiret oneshot(lua::state& l);
	public:
		static void lbind(lua::state& l);
	};
}

#endif /*__LLAE_UV_SIGNAL_H_INCLUDED__*/