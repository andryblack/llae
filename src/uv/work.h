#ifndef __LLAE_UV_WORK_H_INCLUDED__
#define __LLAE_UV_WORK_H_INCLUDED__

#include "req.h"
#include "lua/state.h"
#include "lua/ref.h"

namespace uv {

	class loop;

	class work : public req {
	private:
		uv_work_t m_work;
		static void work_cb(uv_work_t* req);
		static void after_work_cb(uv_work_t* req,int status);
	protected:
        uv_loop_t* get_loop() { return m_work.loop; }
		virtual void on_work() = 0;
		virtual void on_after_work(int satus) = 0;
		work();
		~work();
	public:
		int queue_work(loop& loop);
	};

	class lua_cont_work : public work {
	private:
		lua::ref m_cont;
		void release();
		virtual void on_after_work(int satus) override;
	protected:
		virtual int resume_args(lua::state& l,int status) = 0;
	public:
		lua_cont_work() {}
		explicit lua_cont_work(lua::ref&& cont);
		bool is_active() const { return m_cont.valid(); }
		void reset(lua::state& l);
		int queue_work(lua::state& l);
		int queue_work_thread(lua::state& l);
	};

}

#endif /*__LLAE_UV_WORK_H_INCLUDED__*/
