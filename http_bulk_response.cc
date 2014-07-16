/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "http_bulk_response.hh"
#include "http_record.pb.h"
#include "temp_file.hh"

using namespace std;

HTTPBulkResponse::HTTPBulkResponse()
    : mutex_()
{}

void HTTPBulkResponse::save( const HTTPResponse & response, const Address & server_address )
{
    unique_lock<mutex> ul( mutex_ );
    cout << "Request to " << server_address.ip() << ": " << response.request().first_line() << ", Response: " << response.first_line() << endl;
}
