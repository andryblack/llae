#ifndef __LLAE_UV_WORK_H_INCLUDED__
#define __LLAE_UV_WORK_H_INCLUDED__

#include "req.h"

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

}

#endif /*__LLAE_UV_WORK_H_INCLUDED__*/
