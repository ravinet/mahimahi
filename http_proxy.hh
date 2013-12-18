/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_PROXY_HH
#define HTTP_PROXY_HH

#include <string>
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
    static const std::string client_cert;
    static const std::string server_cert;
    HTTPProxy( const Address & listener_addr, const std::string & record_folder );
    Socket & tcp_listener( void ) { return listener_socket_; }

    void handle_tcp( void );
};

#endif /* HTTP_PROXY_HH */
