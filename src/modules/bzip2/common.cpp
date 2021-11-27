#include "common.h"
#include "llae/diag.h"


extern "C" void bz_internal_error ( int errcode ) {
    char msg[64];
    snprintf(msg, sizeof(msg), "BZ: %d", errcode);
    llae::report_diag_error(msg,__FILE__,__LINE__);
}

void archive::impl::BZLib::pusherror(lua::state& l,stream& z,int z_err) {
    if(z_err == BZ_DATA_ERROR) {
        l.pushstring("Z_DATA_ERROR");
    } else if(z_err == BZ_MEM_ERROR) {
        l.pushstring("Z_MEM_ERROR");
    } else {
        l.pushstring("unknown");
    }
}
