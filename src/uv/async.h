#ifndef __LLAE_UV_ASYNC_H_INCLUDED__
#define __LLAE_UV_ASYNC_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "handle.h"
#include "llae/promise.h"
#include <utility>

namespace llae {
    class loop;
}
namespace uv {
    
    class loop;

	/// @luabind(hidden=true)
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

    /// @luabind(name=async)
    class async_wait : public async {
        META_OBJECT
    private:
        class promise;
        promise* m_promise = nullptr;
    protected:
        virtual void on_async() override;
        void release();
    public:
        explicit async_wait(loop& loop) : async(loop) {}
        
        /// @luabind(name=new)
        static lua::multiret lnew(lua::state& l);
        /// @luabind
        llae::result<> emmit();
        /// @luabind(async=true,name=wait)
        llae::result_promise_ptr<void> wait(llae::loop& l);
        /// @luabind
        void close();
    };
}

#endif /*__LLAE_UV_ASYNC_H_INCLUDED__*/
