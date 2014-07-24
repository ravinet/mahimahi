/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "http_memory_store.hh"
#include "http_record.pb.h"
#include "file_descriptor.hh"
#include "http_header.hh"

using namespace std;

HTTPMemoryStore::HTTPMemoryStore()
    : mutex_(),
      requests(),
      responses()
{}

void HTTPMemoryStore::save( const HTTPResponse & response, const Address & server_address )
{
    unique_lock<mutex> ul( mutex_ );
    cout << "Request to: " << server_address.ip() << endl;

    /* Add the current request to requests BulkMessage and current response to responses BulkMessage */
    requests.add_msg()->CopyFrom( response.request().toprotobuf() );
    responses.add_msg()->CopyFrom( response.toprotobuf() );
}

pair<string, string> HTTPMemoryStore::serialize( void )
{
    unique_lock<mutex> ul( mutex_ );

    /* make sure there is a 1 to 1 matching of requests and responses */
    assert( requests.msg_size() == responses.msg_size() );

    string requests_ret;
    string responses_ret;

    /* Header specifying the response is a bulk response */
    MahimahiProtobufs::HTTPHeader content_type;
    content_type.set_key( "Content-Type" );
    content_type.set_value( "application/x-bulkreply" );

    /* Add bulk response header to both request and response BulkMessage */
    requests.add_header()->CopyFrom( content_type );
    responses.add_header()->CopyFrom( content_type );

    /* Serialize both request and response to String for transmission */
    requests.SerializeToString( &requests_ret );
    responses.SerializeToString( &responses_ret );

    return make_pair( requests_ret, responses_ret );
}
