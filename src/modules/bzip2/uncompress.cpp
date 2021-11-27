#include "uncompress.h"
#include "uv/work.h"
#include "uv/luv.h"
#include "uv/loop.h"
#include "uv/async.h"
#include "uv/fs.h"
#include "llae/app.h"
#include "lua/bind.h"

META_OBJECT_INFO(archive::bzlibuncompress,meta::object)
META_OBJECT_INFO(archive::bzlibuncompress_read,archive::bzlibuncompress)
META_OBJECT_INFO(archive::bzlibuncompress_to_stream,archive::bzlibuncompress)

namespace archive {


    bzlibuncompress::bzlibuncompress() {
    }

    int bzlibuncompress::process(bz_stream* z,int flush,impl::compressionstream<bzlibuncompress,impl::BZLib>& s) {
        int r = ::BZ2_bzDecompress(z);
        return r;
    }

    int bzlibuncompress::init(uv::loop& l,int small) {
        int r = BZ2_bzDecompressInit(&m_z,0,small);
        if (r!=BZ_OK) {
            m_is_error = true;
            return r;
        }
        init_common(l);
        return r;
    }

    bool bzlibuncompress::init_decompress(lua::state& l,int argbase) {
        int small = 0;
        if (l.gettop()>=argbase && l.isboolean(argbase)) {
            small = l.toboolean(argbase) ? 1 : 0;
        }
        int r = init(llae::app::get(l).loop(), small);
        if (r != BZ_OK) {
            l.pushnil();
            impl::BZLib::pusherror(l,m_z,r);
            return false;
        }
        
        return true;
    }
    
    bzlibuncompress::~bzlibuncompress() {
        BZ2_bzDecompressEnd(&m_z);
    }

    


    void bzlibuncompress::lbind(lua::state& l) {
        lua::bind::function(l,"write",&bzlibuncompress::write);
        lua::bind::function(l,"finish",&bzlibuncompress::finish);
        lua::bind::function(l,"send",&bzlibuncompress::send);
    }
    

    bzlibuncompress_read::bzlibuncompress_read() {

    }


    void bzlibuncompress_read::lbind(lua::state& l) {
        lua::bind::function(l,"read",&bzlibuncompress_read::read);
        lua::bind::function(l,"read_buffer",&bzlibuncompress_read::read_buffer);
    }


    lua::multiret bzlibuncompress_read::new_decompress(lua::state& l) {
        bzlibuncompress_read_ptr res(new bzlibuncompress_read());
        if (!res->init_decompress(l,1)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }



    bzlibuncompress_to_stream::bzlibuncompress_to_stream( uv::stream_ptr&& stream ) : compressionstream_to_stream<bzlibuncompress>(std::move(stream)) {

    }

    void bzlibuncompress_to_stream::lbind(lua::state &l) {
        lua::bind::function(l, "shutdown", &bzlibuncompress_to_stream::shutdown);
    }

    lua::multiret bzlibuncompress_to_stream::new_decompress(lua::state& l) {
        auto s = lua::stack<uv::stream_ptr>::get(l,1);
        bzlibuncompress_to_stream_ptr res(new bzlibuncompress_to_stream(std::move(s)));
        if (!res->init_decompress(l,2)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }

   
   
}
