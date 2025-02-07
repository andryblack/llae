#ifndef __LLAE_UV_ASYNC_H_INCLUDED__
#define __LLAE_UV_ASYNC_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "handle.h"
#include "lua/ref.h"
#include <utility>

namespace uv {
    
    class loop;

	class async : public handle {
        META_OBJECT
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
        META_OBJECT
    private:
        lua::ref m_cont;
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
        explicit async_continue(loop& loop) : async(loop) {}
        bool start(lua::ref&& ref) { 
            if (m_cont.valid()) {
                return false;
            }
            m_cont = std::move(ref); 
            return true;
        }
        void reset(lua::state& l) {
            m_cont.reset(l);
        }
        int send();
    };
    using async_continue_ptr = common::intrusive_ptr<async_continue>;

    class async_wait : public async_continue {
        META_OBJECT
    protected:
        explicit async_wait(loop& loop) : async_continue(loop) {}
        virtual int on_cont(lua::state& l) override;
    public:
        static lua::multiret lnew(lua::state& l);
        lua::multiret emmit(lua::state& l);
        lua::multiret wait(lua::state& l);
        static void lbind(lua::state& l);
    };
}

#endif /*__LLAE_UV_ASYNC_H_INCLUDED__*/
