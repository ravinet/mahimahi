/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_MEMORY_STORE_HH
#define HTTP_MEMORY_STORE_HH

#include <string>
#include <mutex>
#include <utility>

#include "http_request.hh"
#include "http_response.hh"
#include "address.hh"
#include "socket.hh"

/* class to print an HTTP request/response to screen */

class HTTPMemoryStore
{
private:
    std::mutex mutex_;
    MahimahiProtobufs::BulkMessage requests;
    MahimahiProtobufs::BulkMessage responses;

public:
    HTTPMemoryStore();
    void save( const HTTPResponse & response, const Address & server_address );
    void serialize_to_socket( Socket && client );
};

#endif /* HTTP_MEMORY_STORE_HH */
