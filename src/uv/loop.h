#ifndef __LLAE_UV_LOOP_H_INCLUDED__
#define __LLAE_UV_LOOP_H_INCLUDED__

#include "decl.h"
#include "llae/loop.h"

namespace uv {

	class loop : public llae::loop {
	private:
		uv_loop_t* m_loop;
		loop(const loop&) = delete;
		loop& operator=(const loop&) = delete;
		loop(loop&&) = delete;
		loop& operator=(loop&&) = delete;
	public:
		explicit loop(uv_loop_t* l);
		~loop();
		uv_loop_t* native() { return m_loop; }
		int run(uv_run_mode mode);
		void stop();
		bool is_alive() const;
		uint64_t now() const;
		static loop default_loop();
	};

}

#endif /*__LLAE_UV_LOOP_H_INCLUDED__*/
