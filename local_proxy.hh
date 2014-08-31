/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef LOCAL_PROXY_HH
#define LOCAL_PROXY_HH

#include <string>

#include "address.hh"
#include "socket.hh"
#include "event_loop.hh"
#include "socket.hh"
#include "secure_socket.hh"
#include "archive.hh"
#include "http_request_parser.hh"

static const std::string could_not_find = "HTTP/1.1 200 OK\r\nContent-Type: Text/html\r\nConnection: close\r\nContent-Length: 24\r\n\r\nCOULD NOT FIND AN OBJECT";

class LocalProxy
{
private:
    Socket listener_socket_;
    Address remote_proxy_addr_;

    Archive archive;

    int add_bulk_requests( const std::string & bulk_requests, std::vector< std::pair< int, int > > & request_positions );

    void handle_response( const std::string & res, const std::vector< std::pair< int, int > > & request_positions, int & response_counter );

    template <class SocketType>
    void handle_new_requests( Poller & client_poller, HTTPRequestParser & request_parser, SocketType && client );

    template <class SocketType1, class SocketType2>
    bool get_response( const HTTPRequest & new_request, const std::string & scheme, SocketType1 && server, bool & already_connected, SocketType2 && client, HTTPRequestParser & request_parser );

    template <class SocketType>
    void handle_client( SocketType && client, const std::string & scheme );

public:
    LocalProxy( const Address & listener_addr, const Address & remote_proxy_addr );

    Socket & tcp_listener( void ) { return listener_socket_; }

    void listen( void );

    void register_handlers( EventLoop & event_loop );

    void serialize_to_socket( Socket && socket_output __attribute__ ((unused)) ) { printf("WARNING NOT IMPLEMENTED\n"); }
};

#endif /* LOCAL_PROXY_HH */
