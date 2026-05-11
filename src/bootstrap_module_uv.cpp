#include "lua/bind.h"
#include "llae/async_bind.h"

#include "../.././src/uv/async.h"
#include "../.././src/uv/fs.h"
#include "../.././src/uv/handle.h"
#include "../.././src/uv/idle.h"
#include "../.././src/uv/luv.h"
#include "../.././src/uv/pipe.h"
#include "../.././src/uv/pipe_server.h"
#include "../.././src/uv/poll.h"
#include "../.././src/uv/process.h"
#include "../.././src/uv/server.h"
#include "../.././src/uv/signal.h"
#include "../.././src/uv/stream.h"
#include "../.././src/uv/tcp_connection.h"
#include "../.././src/uv/tcp_server.h"
#include "../.././src/uv/timer.h"
#include "../.././src/uv/tty.h"
#include "../.././src/uv/udp.h"


/* class uv::async */
static void luabind_uv_async(lua::state& l) {
    
    
    
    
    
}

/* class uv::async_wait */
static void luabind_uv_async_wait(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::async_wait::lnew);
    lua::bind::function(l,"emmit",&uv::async_wait::emmit);
    llae::async_function(l,"wait",&uv::async_wait::wait);
    lua::bind::function(l,"close",&uv::async_wait::close);
    
    
    
}

/* class uv::file */
static void luabind_uv_file(lua::state& l) {
    
    
    lua::bind::function(l,"get_handle",&uv::file::get);
    llae::async_function(l,"close",&uv::file::async_close);
    llae::async_function(l,"fsync",&uv::file::async_fsync);
    llae::async_function(l,"write",&uv::file::lasync_write);
    llae::async_function(l,"read",&uv::file::async_read);
    lua::bind::function(l,"seek",&uv::file::seek);
    lua::bind::function(l,"tell",&uv::file::tell);
    
    
    
}

/* class uv::handle */
static void luabind_uv_handle(lua::state& l) {
    
    
    
    
    
}

/* class uv::idle */
static void luabind_uv_idle(lua::state& l) {
    
    
    
    
    
}

/* class uv::pipe */
static void luabind_uv_pipe(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::pipe::lnew);
    lua::bind::function(l,"connect",&uv::pipe::connect);
    
    
    
}

/* class uv::pipe_server */
static void luabind_uv_pipe_server(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::pipe_server::lnew);
    lua::bind::function(l,"bind",&uv::pipe_server::bind);
    
    
    
}

/* class uv::poll */
static void luabind_uv_poll(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::poll::lnew);
    lua::bind::function(l,"poll",&uv::poll::lpoll);
    lua::bind::function(l,"stop",&uv::poll::lstop);
    
    
    
    lua::bind::value(l, "READABLE", uv::poll::READABLE);
    lua::bind::value(l, "WRITABLE", uv::poll::WRITABLE);
    lua::bind::value(l, "PRIORITIZED", uv::poll::PRIORITIZED);
    lua::bind::value(l, "DISCONNECT", uv::poll::DISCONNECT);
}

/* class uv::process */
static void luabind_uv_process(lua::state& l) {
    
    
    lua::bind::function(l,"spawn",&uv::process::spawn);
    lua::bind::function(l,"kill",&uv::process::kill);
    lua::bind::function(l,"wait_exit",&uv::process::wait_exit);
    
    
    
    lua::bind::value(l, "IGNORE", uv::process::IGNORE);
    lua::bind::value(l, "CREATE_PIPE", uv::process::CREATE_PIPE);
    lua::bind::value(l, "INHERIT_FD", uv::process::INHERIT_FD);
    lua::bind::value(l, "INHERIT_STREAM", uv::process::INHERIT_STREAM);
    lua::bind::value(l, "READABLE_PIPE", uv::process::READABLE_PIPE);
    lua::bind::value(l, "WRITABLE_PIPE", uv::process::WRITABLE_PIPE);
    lua::bind::value(l, "NONBLOCK_PIPE", uv::process::NONBLOCK_PIPE);
    lua::bind::value(l, "PROCESS_DETACHED", uv::process::PROCESS_DETACHED);
}

/* class uv::server */
static void luabind_uv_server(lua::state& l) {
    
    
    llae::async_function(l,"listen",&uv::server::listen);
    lua::bind::function(l,"stop",&uv::server::stop);
    lua::bind::function(l,"accept",&uv::server::accept);
    
    
    
}

/* class uv::signal_base */
static void luabind_uv_signal_base(lua::state& l) {
    
    
    
    
    
}

/* class uv::lua_signal */
static void luabind_uv_lua_signal(lua::state& l) {
    
    
    lua::bind::function(l,"oneshot",&uv::lua_signal::oneshot);
    lua::bind::function(l,"stop",&uv::lua_signal::stop);
    lua::bind::function(l,"unref",&uv::lua_signal::unref);
    
    
    
}

/* class uv::stream */
static void luabind_uv_stream(lua::state& l) {
    
    
    lua::bind::function(l,"write",&uv::stream::lwrite);
    lua::bind::function(l,"read",&uv::stream::read);
    lua::bind::function(l,"shutdown",&uv::stream::shutdown);
    lua::bind::function(l,"send",&uv::stream::send);
    lua::bind::function(l,"add_read_buffer",&uv::stream::add_read_buffer);
    lua::bind::function(l,"close",&uv::stream::close);
    lua::bind::function(l,"stop_read",&uv::stream::stop_read);
    
    
    
}

/* class uv::tcp_connection */
static void luabind_uv_tcp_connection(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::tcp_connection::lnew);
    lua::bind::function(l,"connect",&uv::tcp_connection::connect);
    lua::bind::function(l,"getpeername",&uv::tcp_connection::getpeername);
    lua::bind::function(l,"keepalive",&uv::tcp_connection::keepalive);
    lua::bind::function(l,"nodelay",&uv::tcp_connection::nodelay);
    
    
    
}

/* class uv::tcp_server */
static void luabind_uv_tcp_server(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::tcp_server::lnew);
    lua::bind::function(l,"bind",&uv::tcp_server::bind);
    
    
    
}

/* class uv::timer */
static void luabind_uv_timer(lua::state& l) {
    
    
    
    
    
}

/* class uv::timer_lcb */
static void luabind_uv_timer_lcb(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::timer_lcb::lnew);
    lua::bind::function(l,"start",&uv::timer_lcb::lstart);
    lua::bind::function(l,"stop",&uv::timer_lcb::lstop);
    
    
    
}

/* class uv::timer_wait */
static void luabind_uv_timer_wait(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::timer_wait::lnew);
    lua::bind::function(l,"start",&uv::timer_wait::lstart);
    lua::bind::function(l,"stop",&uv::timer_wait::lstop);
    lua::bind::function(l,"wait",&uv::timer_wait::lwait);
    
    
    
}

/* class uv::tty */
static void luabind_uv_tty(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::tty::lnew);
    lua::bind::function(l,"set_mode",&uv::tty::set_mode);
    lua::bind::function(l,"reset_mode",&uv::tty::reset_mode);
    
    
    
    lua::bind::value(l, "MODE_NORMAL", uv::tty::MODE_NORMAL);
    lua::bind::value(l, "MODE_RAW", uv::tty::MODE_RAW);
    lua::bind::value(l, "MODE_IO", uv::tty::MODE_IO);
}

/* class uv::udp */
static void luabind_uv_udp(lua::state& l) {
    
    
    lua::bind::function(l,"new",&uv::udp::lnew);
    lua::bind::function(l,"bind",&uv::udp::bind);
    lua::bind::function(l,"send",&uv::udp::send);
    lua::bind::function(l,"try_send",&uv::udp::try_send);
    lua::bind::function(l,"connect",&uv::udp::connect);
    lua::bind::function(l,"set_ttl",&uv::udp::set_ttl);
    lua::bind::function(l,"set_broadcast",&uv::udp::set_broadcast);
    lua::bind::function(l,"set_membership",&uv::udp::set_membership);
    lua::bind::function(l,"set_source_membership",&uv::udp::set_source_membership);
    lua::bind::function(l,"set_multicast_loop",&uv::udp::set_multicast_loop);
    lua::bind::function(l,"set_multicast_ttl",&uv::udp::set_multicast_ttl);
    lua::bind::function(l,"set_multicast_interface",&uv::udp::set_multicast_interface);
    lua::bind::function(l,"getpeername",&uv::udp::getpeername);
    lua::bind::function(l,"getsockname",&uv::udp::getsockname);
    lua::bind::function(l,"add_buffer",&uv::udp::add_buffer);
    lua::bind::function(l,"disconnect",&uv::udp::disconnect);
    lua::bind::function(l,"recv",&uv::udp::recv);
    lua::bind::function(l,"stop_recv",&uv::udp::stop_recv);
    
    
    
    lua::bind::value(l, "IPV6ONLY", uv::udp::IPV6ONLY);
    lua::bind::value(l, "PARTIAL", uv::udp::PARTIAL);
    lua::bind::value(l, "REUSEADDR", uv::udp::REUSEADDR);
    lua::bind::value(l, "MMSG_CHUNK", uv::udp::MMSG_CHUNK);
    lua::bind::value(l, "MMSG_FREE", uv::udp::MMSG_FREE);
    lua::bind::value(l, "LINUX_RECVERR", uv::udp::LINUX_RECVERR);
    lua::bind::value(l, "REUSEPORT", uv::udp::REUSEPORT);
    lua::bind::value(l, "RECVMMSG", uv::udp::RECVMMSG);
    lua::bind::value(l, "LEAVE_GROUP", uv::udp::LEAVE_GROUP);
    lua::bind::value(l, "JOIN_GROUP", uv::udp::JOIN_GROUP);
}


int luaopen_uv(lua_State* L) {
    lua::state l(L);
    
    lua::bind::object<uv::file>::register_metatable(l, &luabind_uv_file);
    
    lua::bind::object<uv::handle>::register_metatable(l, &luabind_uv_handle);
    
    lua::bind::object<uv::idle>::register_metatable(l, &luabind_uv_idle);
    
    lua::bind::object<uv::poll>::register_metatable(l, &luabind_uv_poll);
    
    lua::bind::object<uv::process>::register_metatable(l, &luabind_uv_process);
    
    lua::bind::object<uv::server>::register_metatable(l, &luabind_uv_server);
    
    lua::bind::object<uv::signal_base>::register_metatable(l, &luabind_uv_signal_base);
    
    lua::bind::object<uv::lua_signal>::register_metatable(l, &luabind_uv_lua_signal);
    
    lua::bind::object<uv::stream>::register_metatable(l, &luabind_uv_stream);
    
    lua::bind::object<uv::tcp_connection>::register_metatable(l, &luabind_uv_tcp_connection);
    
    lua::bind::object<uv::tcp_server>::register_metatable(l, &luabind_uv_tcp_server);
    
    lua::bind::object<uv::timer>::register_metatable(l, &luabind_uv_timer);
    
    lua::bind::object<uv::timer_lcb>::register_metatable(l, &luabind_uv_timer_lcb);
    
    lua::bind::object<uv::timer_wait>::register_metatable(l, &luabind_uv_timer_wait);
    
    lua::bind::object<uv::tty>::register_metatable(l, &luabind_uv_tty);
    
    lua::bind::object<uv::udp>::register_metatable(l, &luabind_uv_udp);
    
    lua::bind::object<uv::async>::register_metatable(l, &luabind_uv_async);
    
    lua::bind::object<uv::async_wait>::register_metatable(l, &luabind_uv_async_wait);
    
    lua::bind::object<uv::pipe>::register_metatable(l, &luabind_uv_pipe);
    
    lua::bind::object<uv::pipe_server>::register_metatable(l, &luabind_uv_pipe_server);
    
    l.createtable();
    
    lua::bind::object<uv::async_wait>::get_metatable(l);
    l.setfield(-2,"async");
    
    lua::bind::object<uv::file>::get_metatable(l);
    l.setfield(-2,"file");
    
    lua::bind::object<uv::pipe>::get_metatable(l);
    l.setfield(-2,"pipe");
    
    lua::bind::object<uv::pipe_server>::get_metatable(l);
    l.setfield(-2,"pipe_server");
    
    lua::bind::object<uv::poll>::get_metatable(l);
    l.setfield(-2,"poll");
    
    lua::bind::object<uv::process>::get_metatable(l);
    l.setfield(-2,"process");
    
    lua::bind::object<uv::lua_signal>::get_metatable(l);
    l.setfield(-2,"signal");
    
    lua::bind::object<uv::tcp_connection>::get_metatable(l);
    l.setfield(-2,"tcp_connection");
    
    lua::bind::object<uv::tcp_server>::get_metatable(l);
    l.setfield(-2,"tcp_server");
    
    lua::bind::object<uv::timer_lcb>::get_metatable(l);
    l.setfield(-2,"timer");
    
    lua::bind::object<uv::timer_wait>::get_metatable(l);
    l.setfield(-2,"timer_wait");
    
    lua::bind::object<uv::tty>::get_metatable(l);
    l.setfield(-2,"tty");
    
    lua::bind::object<uv::udp>::get_metatable(l);
    l.setfield(-2,"udp");
    
    
    lua::bind::function(l,"exepath",&uv::exepath);
    lua::bind::function(l,"getaddrinfo",&uv::lgetaddrinfo);
    lua::bind::function(l,"cwd",&uv::get_cwd);
    lua::bind::function(l,"chdir",&uv::lchdir);
    lua::bind::function(l,"gettimeofday",&uv::lgettimeofday);
    lua::bind::function(l,"interface_addresses",&uv::linterface_addresses);
    lua::bind::function(l,"set_process_title",&uv::lset_process_title);
    lua::bind::function(l,"random",&uv::lrandom);
    lua::bind::function(l,"print_handles",&uv::lprint_handles);
    lua::bind::function(l,"available_parallelism",&uv_available_parallelism);
    lua::bind::function(l,"cpu_info",&uv::lcpu_info);
    lua::bind::function(l,"ip4_addr",&uv::lip4_addr);
    lua::bind::function(l,"ip6_addr",&uv::lip6_addr);
    lua::bind::function(l,"ip4_name",&uv::lip4_name);
    lua::bind::function(l,"ip6_name",&uv::lip6_name);
    lua::bind::function(l,"if_indextoname",&uv::lif_indextoname);
    lua::bind::function(l,"get_free_memory",&uv_get_free_memory);
    lua::bind::function(l,"get_total_memory",&uv_get_total_memory);
    lua::bind::function(l,"get_constrained_memory",&uv_get_constrained_memory);
    lua::bind::function(l,"get_available_memory",&uv_get_available_memory);
    lua::bind::function(l,"hrtime",&uv_hrtime);
    lua::bind::function(l,"sleep",&uv_sleep);
    llae::async_function(l,"pause",&uv::timer_pause::pause);
    llae::async_function(l,"resume_delayed",&uv::timer_pause::resume_delayed);
    
    
    lua::bind::value(l, "AF_INET", AF_INET);
    lua::bind::value(l, "AF_INET6", AF_INET6);
    lua::bind::value(l, "CLOCK_MONOTONIC", uv::CLOCK_MONOTONIC);
    lua::bind::value(l, "CLOCK_REALTIME", uv::CLOCK_REALTIME);
    return 1;
}
