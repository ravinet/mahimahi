/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_CONSOLE_STORE_HH
#define HTTP_CONSOLE_STORE_HH

#include <string>
#include <mutex>

#include "http_request.hh"
#include "http_response.hh"
#include "address.hh"
#include "socket.hh"

/* class to print an HTTP request/response to screen */

class HTTPConsoleStore
{
private:
    std::mutex mutex_;

public:
    HTTPConsoleStore();
    void save( const HTTPResponse & response, const Address & server_address );
    void serialize_to_socket( Socket && client );
};

#endif /* HTTP_CONSOLE_STORE_HH */
