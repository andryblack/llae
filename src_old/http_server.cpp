#include "http_server.h"

HTTPServer::HTTPServer(uv_loop_t* loop) : TCPServer(loop) {

}

HTTPServer::~HTTPServer() {
	
}