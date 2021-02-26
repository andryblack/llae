#ifndef __LLAE_UV_ASYNC_H_INCLUDED__
#define __LLAE_UV_ASYNC_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "handle.h"
#include "lua/ref.h"

namespace uv {
    
    class loop;

	class async : public handle {
	private:
		uv_async_t m_async;
		static void async_cb(uv_async_t* h);
	protected:
		virtual uv_handle_t* get_handle() override { return reinterpret_cast<uv_handle_t*>(&m_async); }
		virtual void on_async() = 0;
        uv_loop_t* get_loop() { return m_async.loop; }
	public:
		explicit async(loop& loop);
		int send();
	};
	using async_ptr = common::intrusive_ptr<async>;

    class async_continue : public async {
    private:
        lua::ref m_cont;
        void reset(lua::state& l) {
            m_cont.reset(l);
        }
        virtual void on_closed() override;
    protected:
        virtual void on_async() override;
        virtual int on_cont(lua::state& l) {
            return 0;
        }
        virtual void release() {
            m_cont.release();
        }

    public:
        explicit async_continue(loop& loop,lua::ref&& cont) : async(loop), m_cont(std::move(cont)) {}
        int send();
    };
    using async_continue_ptr = common::intrusive_ptr<async_continue>;

}

#endif /*__LLAE_UV_ASYNC_H_INCLUDED__*/
