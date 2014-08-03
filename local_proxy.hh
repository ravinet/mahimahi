/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef LOCAL_PROXY_HH
#define LOCAL_PROXY_HH

#include <string>

#include "address.hh"
#include "socket.hh"
#include "event_loop.hh"
#include "socket.hh"
#include "secure_socket.hh"

class LocalProxy
{
private:
    Socket listener_socket_;
    Address remote_proxy_addr_;

    template <class SocketType>
    void handle_client( SocketType && client, const std::string & scheme );

public:
    LocalProxy( const Address & listener_addr, const Address & remote_proxy_addr );

    Socket & tcp_listener( void ) { return listener_socket_; }

    void listen( EventLoop & event_loop );
};

#endif /* LOCAL_PROXY_HH */
