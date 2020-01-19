#ifndef __LLAE_UV_SIGNAL_H_INCLUDED__
#define __LLAE_UV_SIGNAL_H_INCLUDED__

#include "handle.h"

namespace uv {

	class loop;

	class signal : public handle {
		META_OBJECT
	public:
		virtual uv_handle_t* get_handle() override { return reinterpret_cast<uv_handle_t*>(&m_sig); }
	private:
		uv_signal_t m_sig;
		static void signal_cb(uv_signal_t* sig, int signum);
	public:
		signal(loop& l);
	};

}

#endif /*__LLAE_UV_SIGNAL_H_INCLUDED__*/