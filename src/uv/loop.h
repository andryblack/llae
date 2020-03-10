#ifndef __LLAE_UV_LOOP_H_INCLUDED__
#define __LLAE_UV_LOOP_H_INCLUDED__

#include "meta/object.h"
#include "decl.h"
#include "lua/state.h"

namespace uv {

	class loop : public meta::object {
		META_OBJECT
	private:
		uv_loop_t* m_loop;
	public:
		explicit loop(uv_loop_t* l);
		~loop();
		uv_loop_t* native() { return m_loop; }
		int run(uv_run_mode mode);
		void stop();
		bool is_alive() const;
		uint64_t now() const;

		static loop* self(uv_loop_t* l) { return static_cast<loop*>(l->data); }
	};

}

#endif /*__LLAE_UV_LOOP_H_INCLUDED__*/