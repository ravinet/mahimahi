/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "http_console_store.hh"
#include "http_record.pb.h"
#include "temp_file.hh"

using namespace std;

HTTPConsoleStore::HTTPConsoleStore()
    : mutex_()
{}

void HTTPConsoleStore::save( const HTTPResponse & response, const Address & server_address )
{
    unique_lock<mutex> ul( mutex_ );
    cout << "Request to " << server_address.ip() << ": " << response.request().first_line() << ", Response: " << response.first_line() << endl;
}

void HTTPConsoleStore::serialize_to_socket( Socket && client __attribute__ ((unused)), MahimahiProtobufs::BulkRequest & bulk_request  __attribute__ ((unused)) ) {}
