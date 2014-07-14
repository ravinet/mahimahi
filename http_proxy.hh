/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_PROXY_HH
#define HTTP_PROXY_HH

#include <string>

#include "socket.hh"
#include "secure_socket.hh"
#include "http_response.hh"

class HTTPBackingStore;
class EventLoop;
class Poller;
class HTTPRequestParser;
class HTTPResponseParser;

class HTTPProxy
{
private:
    Socket listener_socket_;

    template <class SocketType>
    void loop( SocketType & server, SocketType & client );

    SSLContext server_context_, client_context_;

    HTTPBackingStore & backing_store_;

public:
    HTTPProxy( const Address & listener_addr, HTTPBackingStore & backing_store );

    Socket & tcp_listener( void ) { return listener_socket_; }

    void handle_tcp( void );

    void register_handlers( EventLoop & event_loop );
};

#endif /* HTTP_PROXY_HH */
