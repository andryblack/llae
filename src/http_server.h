#ifndef __LLAE_HTTP_SERVER_H_INCLUDED__
#define __LLAE_HTTP_SERVER_H_INCLUDED__

#include "llae.h"
#include "tcp_server.h"

class HTTPServer : public TCPServer {
private:

public:
	explicit HTTPServer(uv_loop_t* loop);
	~HTTPServer();
};

#endif /*__LLAE_HTTP_SERVER_H_INCLUDED__*/