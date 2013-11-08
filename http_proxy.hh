/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_PROXY_HH
#define HTTP_PROXY_HH

#include <fstream>

#include "socket.hh"

class HTTPProxy
{
private:
    Socket listener_socket_;
    std::fstream original_requests_;
    std::fstream sent_requests_;


public:
    HTTPProxy( const Address & listener_addr );
    Socket & tcp_listener( void ) { return listener_socket_; }

    void handle_tcp( void );
};

#endif /* HTTP_PROXY_HH */
