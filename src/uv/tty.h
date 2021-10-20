#ifndef __LLAE_UV_TTY_H_INCLUDED__
#define __LLAE_UV_TTY_H_INCLUDED__

#include "stream.h"
#include "lua/state.h"
#include "lua/ref.h"

namespace uv {

	class loop;
	
	class tty : public stream {
		META_OBJECT
	private:
		uv_tty_t m_tty;
	public:
		virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_tty); }
		virtual uv_stream_t* get_stream() override final { return reinterpret_cast<uv_stream_t*>(&m_tty); }
	protected:
		virtual ~tty() override;
	public:
		explicit tty(uv::loop& loop,uv_file fd );
		static lua::multiret lnew(lua::state& l,uv_file fd);
		static void lbind(lua::state& l);

		lua::multiret set_mode(lua::state& l,uv_tty_mode_t mode);
		static lua::multiret reset_mode(lua::state& l);
	};
	typedef common::intrusive_ptr<tty> tty_ptr;

}

#endif /*__LLAE_UV_TTY_H_INCLUDED__*/
