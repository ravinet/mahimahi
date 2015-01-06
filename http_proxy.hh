/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_PROXY_HH
#define HTTP_PROXY_HH

#include <string>

#include "socket.hh"
#include "secure_socket.hh"
#include "http_response.hh"

class EventLoop;
class Poller;
class HTTPRequestParser;
class HTTPResponseParser;

template <class StoreType>
class HTTPProxy
{
private:
    StoreType backing_store_;

    Socket listener_socket_;

    template <class SocketType>
    void loop( SocketType & server, SocketType & client );

    SSLContext server_context_, client_context_;

public:
    template <typename... Targs>
    HTTPProxy( const Address & listener_addr, Targs... Fargs );

    Socket & tcp_listener( void ) { return listener_socket_; }

    void handle_tcp();

    /* register this HTTPProxy's TCP listener socket to handle events with
       the given event_loop, saving request-response pairs to the given
       backing_store (which is captured and must continue to persist) */
    void register_handlers( EventLoop & event_loop );

    /* Serialize results to socket */
    void serialize_to_socket( Socket && socket_output, MahimahiProtobufs::BulkRequest bulk_request ) { backing_store_.serialize_to_socket( std::move( socket_output ), bulk_request ); }

    void print_sent_requests( void );
};

#endif /* HTTP_PROXY_HH */
