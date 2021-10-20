#include "poll.h"

#include "lua/stack.h"
#include "lua/bind.h"
#include "llae/app.h"
#include "luv.h"

#include <iostream>

META_OBJECT_INFO(uv::poll,uv::handle)


namespace uv {

	class lua_poll_consumer : public poll_consumer {
    private:
        lua::ref m_cont;
    public:
        lua_poll_consumer(lua::ref && cont) : m_cont(std::move(cont)) {}
        virtual bool on_poll(poll* p, int status,int events) override final {
            auto& l = llae::app::get(p->get_handle()->loop).lua();
            if (m_cont.valid()) {
                m_cont.push(l);
                auto toth = l.tothread(-1);
                l.pop(1);// thread
                 if (status < 0) {
                    toth.pushnil();
                    uv::push_error(toth,status);
                } else { 
                    toth.pushinteger(events);
                }
                lua::ref ref(std::move(m_cont));
                p->stop_poll();
                auto s = toth.resume(l,2);
                if (s != lua::status::ok && s != lua::status::yield) {
                    llae::app::show_error(toth,s);
                }
                ref.reset(l);
            } else {
                p->stop_poll();
            }
            return true;
        }
        void on_poll_closed(poll* p) override final {
        	auto& l = llae::app::get(p->get_handle()->loop).lua();
            if (!l.native()) {
                m_cont.release();
            }
        	if (m_cont.valid()) {
        		m_cont.push(l);
                auto toth = l.tothread(-1);
                l.pop(1);// thread
                toth.pushnil();
                toth.pushnil();
                lua::ref ref(std::move(m_cont));
                auto s = toth.resume(l,2);
                if (s != lua::status::ok && s != lua::status::yield) {
                    llae::app::show_error(toth,s);
                }
                ref.reset(l);
        	}
        }
    };

	poll::poll(loop& l,int fd) {
		int r = uv_poll_init(l.native(),&m_poll,fd);
		UV_DIAG_CHECK(r);
		attach();
	}
	poll::~poll() {
	}

	lua::multiret poll::lpoll(lua::state& l, int events) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("poll::lpoll is async");
			return {2};
		}
		if (m_consumer) {
			l.pushnil();
			l.pushstring("poll::lpoll already poll");
			return {2};
		}
		{
			l.pushthread();
            lua::ref poll_cont;
			poll_cont.set(l);

            common::intrusive_ptr<lua_poll_consumer> consume(new lua_poll_consumer(std::move(poll_cont)));
            int res = start_poll(events,consume);
            if (res < 0) {
                l.pushnil();
                uv::push_error(l,res);
                return {2};
            }
			
		}
		l.yield(0);
		return {0};
	}

	int poll::start_poll(int events, const poll_consumer_ptr& consumer) {
		if (m_consumer) {
            return -1;
        }
        m_consumer = consumer;
        add_ref();
        int res = uv_poll_start(&m_poll, events, &poll::on_poll_cb);
        if (res < 0) {
            m_consumer.reset();
            //std::cout << "stream start_read error remove_ref" << std::endl;
            remove_ref();
        }
        return res;
	}
	int poll::stop_poll() {
		return uv_poll_stop(&m_poll);
	}

	bool poll::on_poll(int status, int events) {
		if (m_consumer) {
            auto consumer = std::move(m_consumer);
            bool res = consumer->on_poll(this,status,events);
            if (!res) {
                m_consumer = std::move(consumer);
            }
            return res;
        } else {
            LLAE_DIAG(std::cout << "poll callback without consumer" << std::endl;)
            stop_poll();
        }
        return true;
	}
	void poll::on_poll_cb(uv_poll_t *handle, int status, int events) {
		poll* self = static_cast<poll*>(handle->data);
        if (self->on_poll(status,events)) {
            //std::cout << "stream read_cb remove_ref" << std::endl;
            self->remove_ref();
		}
	}
	void poll::on_closed() {
		//LLAE_DIAG(std::cout << "stream::on_closed" << std::endl;)
		//m_closed = true;
        if (m_consumer) {
            m_consumer->on_poll_closed(this);
            m_consumer.reset();
        }
        handle::on_closed();
	}
		
	void poll::lbind(lua::state& l) {
		lua::bind::function(l,"poll",&poll::lpoll);

		l.pushinteger(UV_READABLE);
		l.setfield(-2,"READABLE");
		l.pushinteger(UV_WRITABLE);
		l.setfield(-2,"WRITABLE");
		l.pushinteger(UV_PRIORITIZED);
		l.setfield(-2,"PRIORITIZED");
		l.pushinteger(UV_DISCONNECT);
		l.setfield(-2,"DISCONNECT");
		
	}
}
