#include "pipe.h"
#include "luv.h"
#include "loop.h"
#include "llae/app.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::pipe,uv::stream)


namespace uv {

    class pipe::connect_req : public req {
    private:
        uv_connect_t m_req;
        pipe_ptr m_conn;
        lua::ref m_cont;
    public:
        connect_req(pipe_ptr&& con,lua::ref&& cont) : m_conn(std::move(con)),m_cont(std::move(cont)) {
            attach(reinterpret_cast<uv_req_t*>(get()));
        }
        uv_connect_t* get() { return &m_req; }
        static void connect_cb(uv_connect_t* req, int status) {
            auto self = static_cast<connect_req*>(req->data);
            self->on_end(status);
            self->remove_ref();
        }
        void connect(const char* file) {
            add_ref();
            uv_pipe_connect(&m_req,m_conn->get_pipe(),file,&connect_req::connect_cb);
        }
        void reset(lua::state& l) {
            m_cont.reset(l);
        }
        void on_end(int status) {
            if (llae::app::closed(m_conn->get_handle()->loop)) {
                m_cont.release();
                return;
            }
            auto& l = llae::app::get(m_conn->get_handle()->loop).lua();
            if (!l.native()) {
                m_cont.release();
                return;
            }
            l.checkstack(2);
            m_cont.push(l);
            auto toth = l.tothread(-1);
            toth.checkstack(3);
            int nargs;
            if (status < 0) {
                toth.pushnil();
                uv::push_error(toth,status);
                nargs = 2;
            } else {
                toth.pushboolean(true);
                nargs = 1;
            }
            auto s = toth.resume(toth,nargs);
            if (s != lua::status::ok && s != lua::status::yield) {
                llae::app::show_error(toth,s);
            }
            l.pop(1);// thread
            m_cont.reset(l);
        }
    };

    pipe::pipe(loop& l,int ipc) {
        int r = uv_pipe_init(l.native(),&m_pipe,ipc);
        UV_DIAG_CHECK(r);
        attach();
    }

    pipe::~pipe() {
    }


    lua::multiret pipe::lnew(lua::state& l) {
        int ipc = l.optinteger(1,0);
        common::intrusive_ptr<pipe> res{new pipe(llae::app::get(l).loop(),ipc)};
        lua::push(l,std::move(res));
        return {1};
    }

    lua::multiret pipe::connect(lua::state& l) {
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("pipe::connect is async");
            return {2};
        }
        
        {
            const char* file = l.checkstring(2);

            l.pushthread();
            lua::ref connect_cont;
            connect_cont.set(l);
        
            common::intrusive_ptr<connect_req> req{new connect_req(pipe_ptr(this),
                std::move(connect_cont))};
            req->connect(file);
        } 
        l.yield(0);
        return {0};
    }

        
    void pipe::lbind(lua::state& l) {
        lua::bind::function(l,"new",&pipe::lnew);
        lua::bind::function(l,"connect",&pipe::connect);
    }
}

