#pragma once

#include "handle.h"
#include "common/intrusive_ptr.h"


namespace uv {

	class loop;

	class idle : public handle {
		META_OBJECT
	public:
		virtual uv_handle_t* get_handle() override { return reinterpret_cast<uv_handle_t*>(&m_idle); }
	private:
		uv_idle_t m_idle;
		bool m_start_ref = false;
		void unref();
		static void idle_cb(uv_idle_t *handle);
	protected:
		virtual void on_cb() = 0;
	public:
		explicit idle(loop& l);
		~idle();
		int stop();
		int start();
	};
	using idle_ptr = common::intrusive_ptr<idle>;

}

