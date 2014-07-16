/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_BULK_RESPONSE_HH
#define HTTP_BULK_RESPONSE_HH

#include <string>
#include <mutex>

#include "http_request.hh"
#include "http_response.hh"
#include "address.hh"
#include "backing_store.hh"

/* class to print an HTTP request/response to screen */

class HTTPBulkResponse : public HTTPBackingStore
{
private:
    std::mutex mutex_;

public:
    HTTPBulkResponse();
    void save( const HTTPResponse & response, const Address & server_address ) override;
};

#endif /* HTTP_BULK_RESPONSE_HH */
