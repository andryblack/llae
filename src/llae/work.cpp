#include "work.h"
#include "common/intrusive_ptr.h"
#include "llae/app.h"
#include "meta/object.h"
#include "uv/work.h"
#include "uv/luv.h"

META_OBJECT_INFO(llae::work_base, meta::object)

namespace llae {

    class work_base::work_impl : public uv::work {
    private:
        work_base_ptr m_work;
    public:
        explicit work_impl(work_base_ptr&& work) : m_work(std::move(work)) {}
        virtual void on_work() override {
            m_work->do_work();
        }
        virtual void on_after_work(int status) override {
            uv::loop l{get_loop()};
            if (status != 0) {
                m_work->on_after_work(l, common::make_intrusive<uv::status_error>(status));
            } else {
                m_work->on_after_work(l,{});
            }
            m_work.reset();
        }
    };

    error_ptr work_base::schedule_work(loop& a) {
        auto impl = common::make_intrusive<work_impl>(work_base_ptr(this));
        auto r = impl->queue_work(static_cast<uv::loop&>(a));
        if (r != 0) {
            return common::make_intrusive<uv::status_error>(r);
        }
        return {};
    }

}