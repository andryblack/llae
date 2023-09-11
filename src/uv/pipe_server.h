#ifndef __LLAE_UV_PIPE_SERVER_H_INCLUDED__
#define __LLAE_UV_PIPE_SERVER_H_INCLUDED__

#include "server.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "posix/fd.h"

namespace uv {

    class loop;
    
    class pipe_server : public server {
        META_OBJECT
    private:
        uv_pipe_t m_pipe;
    public:
        virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_pipe); }
        virtual uv_stream_t* get_stream() override final { return reinterpret_cast<uv_stream_t*>(&m_pipe); }
    protected:
        virtual ~pipe_server() override;
    public:
        explicit pipe_server(loop& l,int ipc);
        
        static lua::multiret lnew(lua::state& l);
        static void lbind(lua::state& l);

        lua::multiret bind(lua::state& l);
    };
    using pipe_server_ptr = common::intrusive_ptr<pipe_server>;

}

#endif /*__LLAE_UV_PIPE_SERVER_H_INCLUDED__*/

