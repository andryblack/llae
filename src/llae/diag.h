#ifndef __LLAE_DIAG_H_INCLUDED__
#define __LLAE_DIAG_H_INCLUDED__

#include <new>

#ifndef LLAE_SELF_DIAG
#ifndef LLAE_NO_SELF_DIAG
#define LLAE_SELF_DIAG
#endif
#endif

namespace llae {
	void report_diag_error(const char* msg,const char* file,int line);
}

#ifdef LLAE_SELF_DIAG
#define LLAE_DIAG(X) X
#define LLAE_CHECK(cond) do{ if (!(cond)) ::llae::report_diag_error(#cond,__FILE__,__LINE__); } while(false);
#else
#define LLAE_DIAG(X)
#define LLAE_CHECK(X)
#endif

#endif /*__LLAE_DIAG_H_INCLUDED__*/
