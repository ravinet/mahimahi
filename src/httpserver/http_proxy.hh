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
    TCPSocket listener_socket_;

    template <class SocketType>
    void loop( SocketType & server, SocketType & client, HTTPBackingStore & backing_store );

    SSLContext server_context_, client_context_;

public:
    HTTPProxy( const Address & listener_addr );

    TCPSocket & tcp_listener( void ) { return listener_socket_; }

    void handle_tcp( HTTPBackingStore & backing_store );

    /* register this HTTPProxy's TCP listener socket to handle events with
       the given event_loop, saving request-response pairs to the given
       backing_store (which is captured and must continue to persist) */
    void register_handlers( EventLoop & event_loop, HTTPBackingStore & backing_store );
};

#endif /* HTTP_PROXY_HH */
