/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "http_memory_store.hh"
#include "http_record.pb.h"
#include "file_descriptor.hh"
#include "http_header.hh"
#include "int32.hh"
#include "file_descriptor.hh"

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
    /* bulk response format: # of pairs, request protobuf size, request protobuf, # of pairs, response protobuf size, response protobuf */

    unique_lock<mutex> ul( mutex_ );

    /* make sure there is a 1 to 1 matching of requests and responses */
    assert( requests.msg_size() == responses.msg_size() );

    /* Number of request/response pairs (4 bytes) */
    string pairs = static_cast<string>( Integer32( requests.msg_size() ) );

    /* Serialize both request and response to String for transmission */
    string all_requests;
    string all_responses;
    requests.SerializeToString( &all_requests );
    responses.SerializeToString( &all_responses );

    /* Put sizes and messages into bulk response formats (one for request, one for response) */
    string requests_ret = pairs + static_cast<string>( Integer32( all_requests.size() ) ) + all_requests;
    string responses_ret = pairs + static_cast<string>( Integer32( all_responses.size() ) ) + all_responses;

    return make_pair( requests_ret, responses_ret );
}

void HTTPMemoryStore::persist( void )
{
    FileDescriptor bulkreply = SystemCall( "open", open( "bulk_reply.txt", O_WRONLY | O_CREAT, 00700 ) );
    auto bulk_message = serialize();
    /* Write requests first to ensure that they arrive quickly for parsing */
    bulkreply.write( bulk_message.first );
    bulkreply.write( bulk_message.second );
}
