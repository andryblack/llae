#ifndef __LLAE_UV_PIPE_H_INCLUDED__
#define __LLAE_UV_PIPE_H_INCLUDED__

#include "stream.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "posix/fd.h"

namespace uv {

    class loop;
    
    /// @luabind
    class pipe : public stream {
        META_OBJECT
    private:
        uv_pipe_t m_pipe;
        class connect_req;
    public:
        virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_pipe); }
        virtual uv_stream_t* get_stream() override final { return reinterpret_cast<uv_stream_t*>(&m_pipe); }
    protected:
        virtual ~pipe() override;
        uv_pipe_t* get_pipe() { return &m_pipe; }
    public:
        explicit pipe(uv::loop& loop,int ipc);
        /// @luabind(name=new)
        static lua::multiret lnew(lua::state& l);
        /// @luabind
        lua::multiret connect(lua::state& l);
    };
    typedef common::intrusive_ptr<pipe> pipe_ptr;

}

#endif /*__LLAE_UV_PIPE_H_INCLUDED__*/

