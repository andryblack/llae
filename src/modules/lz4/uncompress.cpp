#include "uncompress.h"
#include "uv/work.h"
#include "uv/luv.h"
#include "uv/loop.h"
#include "uv/async.h"
#include "uv/fs.h"
#include "llae/app.h"
#include "lua/bind.h"

META_OBJECT_INFO(archive::lz4uncompress,meta::object)
META_OBJECT_INFO(archive::lz4uncompress_read,archive::lz4uncompress)
META_OBJECT_INFO(archive::lz4uncompress_to_stream,archive::lz4uncompress)

namespace archive {


    void impl::LZ4DF::pusherror(lua::state& l,stream& z,int z_err) {
        l.pushstring(LZ4F_getErrorName(z_err));
    }

    lz4uncompress::lz4uncompress() {
        LZ4F_createDecompressionContext(&m_z.ctx,LZ4F_VERSION);
        m_z.options.stableDst = 0;
        m_z.options.skipChecksums = 0;
    }

    int lz4uncompress::process(impl::LZ4DF::stream* z,int flush,impl::compressionstream<lz4uncompress,impl::LZ4DF>& s) {
        size_t out_size = z->avail_out;
        size_t in_size = z->avail_in;
        if (in_size == 0) {
            if (flush == impl::LZ4DF::FINISH) {
                return impl::LZ4DF::STREAM_END;
            } else {
                return impl::LZ4DF::BUF_ERROR;
            }
        }
        
        auto sz = LZ4F_decompress(z->ctx,z->next_out,&out_size,z->next_in,&in_size,&z->options);
        if (LZ4F_isError(sz)) {
            return int(sz);
        }
        z->next_out += out_size;
        z->next_in += in_size;
        z->avail_out -= out_size;
        z->avail_in -= in_size;

        if (sz == 0 && flush == impl::LZ4DF::FINISH) {
            return impl::LZ4DF::STREAM_END;
        }
        return 0;
    }

    int lz4uncompress::init(uv::loop& l) {
        LZ4F_resetDecompressionContext(m_z.ctx);
        init_common(l);
        return 0;
    }

    bool lz4uncompress::init_decompress(lua::state& l,int argbase) {
        init(llae::app::get(l).loop());
        return true;
    }
    
    lz4uncompress::~lz4uncompress() {
        LZ4F_freeDecompressionContext(m_z.ctx);
    }

    


    void lz4uncompress::lbind(lua::state& l) {
        lua::bind::function(l,"write",&lz4uncompress::write);
        lua::bind::function(l,"finish",&lz4uncompress::finish);
        lua::bind::function(l,"send",&lz4uncompress::send);
    }
    

    lz4uncompress_read::lz4uncompress_read() {

    }


    void lz4uncompress_read::lbind(lua::state& l) {
        lua::bind::function(l,"read",&lz4uncompress_read::read);
        lua::bind::function(l,"read_buffer",&lz4uncompress_read::read_buffer);
    }


    lua::multiret lz4uncompress_read::new_decompress(lua::state& l) {
        lz4uncompress_read_ptr res(new lz4uncompress_read());
        if (!res->init_decompress(l,1)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }



    lz4uncompress_to_stream::lz4uncompress_to_stream( uv::stream_ptr&& stream ) : compressionstream_to_stream<lz4uncompress>(std::move(stream)) {

    }

    void lz4uncompress_to_stream::lbind(lua::state &l) {
        lua::bind::function(l, "shutdown", &lz4uncompress_to_stream::shutdown);
    }

    lua::multiret lz4uncompress_to_stream::new_decompress(lua::state& l) {
        auto s = lua::stack<uv::stream_ptr>::get(l,1);
        lz4uncompress_to_stream_ptr res(new lz4uncompress_to_stream(std::move(s)));
        if (!res->init_decompress(l,2)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }

   
   
}
