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

class HTTPProxy
{
private:
    Socket listener_socket_;

    /* folder to store recorded http content in */
    std::string record_folder_;

    /* Pick a random file name and store reqrespair as a serialized string */
    void save_to_disk( const HTTPResponse & response, const Address & server_address );

    SSLContext server_context_, client_context_;

public:
    HTTPProxy( const Address & listener_addr, const std::string & record_folder );

    Socket & tcp_listener( void ) { return listener_socket_; }

    void handle_tcp( void );

    void register_handlers( EventLoop & event_loop );

    template <class SocketType>
    void loop( SocketType & server, SocketType & client );
};

#endif /* HTTP_PROXY_HH */
