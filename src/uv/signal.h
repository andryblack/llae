#ifndef __LLAE_UV_SIGNAL_H_INCLUDED__
#define __LLAE_UV_SIGNAL_H_INCLUDED__

#include "handle.h"
#include <functional>

namespace uv {

	class loop;

	class signal : public handle {
		META_OBJECT
	public:
		virtual uv_handle_t* get_handle() override { return reinterpret_cast<uv_handle_t*>(&m_sig); }
	private:
		uv_signal_t m_sig;
		static void signal_cb(uv_signal_t* sig, int signum);
		std::function<void()> m_cb;
	public:
		signal(loop& l);
		~signal();
		int start(int signum,std::function<void()>&& f);
		int start_oneshot(int signum,std::function<void()>&& f);
	};

}

#endif /*__LLAE_UV_SIGNAL_H_INCLUDED__*/