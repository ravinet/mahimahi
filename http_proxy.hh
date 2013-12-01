/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_PROXY_HH
#define HTTP_PROXY_HH

#include <queue>

#include "socket.hh"

class HTTPProxy
{
private:
    Socket listener_socket_;
    /* folder to store recorded http content in */
    std::string record_folder_;

public:
    HTTPProxy( const Address & listener_addr, const std::string & record_folder );
    Socket & tcp_listener( void ) { return listener_socket_; }

    void handle_tcp( void );
};

#endif /* HTTP_PROXY_HH */
