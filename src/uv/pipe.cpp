#include "pipe.h"
#include "luv.h"
#include "loop.h"
#include "llae/app.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::pipe,uv::stream)


namespace uv {

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

        
    void pipe::lbind(lua::state& l) {
        lua::bind::function(l,"new",&pipe::lnew);
    }
}

