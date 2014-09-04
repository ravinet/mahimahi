/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BACKING_STORE_HH
#define BACKING_STORE_HH

#include <string>
#include <mutex>

#include "http_request.hh"
#include "http_response.hh"
#include "address.hh"
#include "socket.hh"

class HTTPDiskStore
{
private:
    std::string record_folder_;
    std::mutex mutex_;

public:
    HTTPDiskStore( const std::string & record_folder );
    void save( const HTTPResponse & response, const Address & server_address );
    void serialize_to_socket( Socket && client );
};

#endif /* BACKING_STORE_HH */
