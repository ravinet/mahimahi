/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_CONSOLE_STORE_HH
#define HTTP_CONSOLE_STORE_HH

#include <string>
#include <mutex>

#include "http_request.hh"
#include "http_response.hh"
#include "address.hh"
#include "backing_store.hh"

/* class to print an HTTP request/response to screen */

class HTTPConsoleStore : public HTTPBackingStore
{
private:
    std::mutex mutex_;

public:
    HTTPConsoleStore();
    void save( const HTTPResponse & response, const Address & server_address ) override;
    void serialize_to_socket( Socket && client ) override;
};

#endif /* HTTP_CONSOLE_STORE_HH */
