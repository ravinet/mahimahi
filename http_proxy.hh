/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_PROXY_HH
#define HTTP_PROXY_HH

#include <queue>

#include "secure_socket.hh"
#include "socket.hh"
#include "http_record.pb.h"
#include "http_response.hh"

class HTTPProxy
{
private:
    Socket listener_socket_;
    /* folder to store recorded http content in */
    std::string record_folder_;

    /* Pick a random file name and store reqrespair as a serialized string */
    void reqres_to_protobuf( HTTP_Record::reqrespair & current_pair, const HTTPResponse & response );

public:
    struct SslPair {
        std::unique_ptr<Secure_Socket> ssl_client {nullptr};
        std::unique_ptr<Secure_Socket> ssl_server {nullptr};
    public:
        bool is_null() { return ssl_client == nullptr; }
    };

    HTTPProxy( const Address & listener_addr, const std::string & record_folder );
    Socket & tcp_listener( void ) { return listener_socket_; }

    SslPair make_ssl_pair( Socket & client, Socket & server, const int & dst_port );

    void handle_tcp( void );
};

#endif /* HTTP_PROXY_HH */
