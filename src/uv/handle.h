#ifndef __LLAE_UV_HANDLE_H_INCLUDED__
#define __LLAE_UV_HANDLE_H_INCLUDED__

#include "meta/object.h"
#include "decl.h"

namespace uv {

	class handle : public meta::object {
		META_OBJECT
	public:
		virtual uv_handle_t* get_handle() = 0;
	private:
		static void close_cb(uv_handle_t* handle);
		static void close_destroy_cb(uv_handle_t* handle);
		void destroy() override;
	protected:
		handle();
		void attach();
		virtual ~handle() override;
		virtual void on_closed() {}
	public:
		void close();
	};

}

#endif /*__LLAE_UV_HANDLE_H_INCLUDED__*/