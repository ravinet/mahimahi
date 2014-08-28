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

void HTTPMemoryStore::serialize_to_socket( Socket && client )
{
    /* bulk response format: # of pairs, request protobuf size, request protobuf, # of pairs, response protobuf size, response protobuf */

    unique_lock<mutex> ul( mutex_ );

    /* make sure there is a 1 to 1 matching of requests and responses */
    assert( requests.msg_size() == responses.msg_size() );

    /* Serialize both request and response to String for transmission */
    string all_requests;
    requests.SerializeToString( &all_requests );

    /* Put sizes and messages into bulk response formats (one for request, one for response) */
    string requests_ret = static_cast<string>( Integer32( all_requests.size() ) ) + all_requests;

    string responses_ret;

    for ( int i = 0; i < responses.msg_size(); i++ ) { /* add response string size and response string */
        string current_response;
        responses.msg( i ).SerializeToString( &current_response );
        responses_ret = responses_ret + static_cast<string>( Integer32( current_response.size() ) ) + current_response;
    }

    /* prepend the first response size and the response itself to the requests of bulk response */
    string first_response;
    responses.msg( 0 ).SerializeToString( &first_response );
    string requests_and_response = static_cast<string>( Integer32( first_response.size() ) ) + first_response + requests_ret;

    client.write( requests_and_response );
    client.write( responses_ret );
}
