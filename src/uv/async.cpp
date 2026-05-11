#include "async.h"
#include "loop.h"
#include "llae/app.h"
#include "luv.h"

META_OBJECT_INFO(uv::async,uv::handle)
META_OBJECT_INFO(uv::async_wait,uv::async)

namespace uv {

	async::async(loop& loop) {
		uv_async_init(loop.native(),&m_async,&async::async_cb);
		attach();
	}

	void async::async_cb(uv_async_t* h) {
		async_ptr self{static_cast<async*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(h)))};
		self->on_async();
	}

	int async::send() {
		return uv_async_send(&m_async);
	}


    class async_wait::promise : public llae::result_promise<void> {
    private:
        common::intrusive_ptr<async_wait> m_async;
    public:
        promise(async_wait* async) : llae::result_promise<void>(), m_async(async) {}
        ~promise() override {
            m_async->release();
            m_async.reset();
        }
        void resolve() {
            if (m_async) {
                auto async = std::move(m_async);
                m_async.reset();
                async->release();
                set_result(llae::result<>());
            }
        }
    };

    lua::multiret async_wait::lnew(lua::state& l) {
        lua::push(l,common::intrusive_ptr<async_wait>(new async_wait(llae::app::get(l).loop())));
        return {1};
    }
    llae::result<> async_wait::emmit() {
        auto res = send();
        return make_result(res);
    }

    void async_wait::release() {
        m_promise = nullptr;
    }

    void async_wait::on_async() {
        auto hold = common::intrusive_ptr<async_wait>(this);
        if (m_promise) {
            m_promise->resolve();
        }
    }

    llae::result_promise_ptr<void> async_wait::wait(llae::loop& l) {
        if (is_closing()) {
            return llae::result_promise_forward_error<void>(llae::string_error::create("async_wait closing"));
        }
        if (m_promise) {
            return llae::result_promise_ptr<void>(m_promise);
        }
        auto p = common::make_intrusive<promise>(this);
        m_promise = p.get();
        return p;
    }

    void async_wait::close() {
        if (m_promise) {
            auto p = common::intrusive_ptr<promise>(m_promise);
            m_promise = nullptr;
            p->set_result(llae::string_error::create("async_wait closed"));
        }
        async::close();
    }


}
