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

void HTTPMemoryStore::save( const HTTPResponse & response, const Address & server_address __attribute__ ( ( unused ) ) )
{
    unique_lock<mutex> ul( mutex_ );

    /* Add the current request to requests BulkMessage and current response to responses BulkMessage */
    requests.add_msg()->CopyFrom( response.request().toprotobuf() );
    responses.add_msg()->CopyFrom( response.toprotobuf() );
}

void HTTPMemoryStore::serialize_to_socket( Socket && client )
{
    /* bulk response format:
       length-value pair for first response,
       length-value pair for requests protobuf,
       for each response:
         length-value pair for each response
     */
    unique_lock<mutex> ul( mutex_ );

    /* make sure there is a 1 to 1 matching of requests and responses */
    assert( requests.msg_size() == responses.msg_size() );

    /* First, write out the first response as a length-value pair */
    string first_response;
    responses.msg( 0 ).SerializeToString( &first_response );
    first_response = static_cast<string>( Integer32( first_response.size() ) ) + first_response;

    /* Serialize all requests alone to a length-value pair for transmission */
    string all_requests;
    requests.SerializeToString( &all_requests );
    all_requests = static_cast<string>( Integer32( all_requests.size() ) ) + all_requests;

    /* Now make a set of length-value pairs, one for each response */
    string all_responses;
    for ( int i = 0; i < responses.msg_size(); i++ ) {
        /* add response string size and response string */
        string current_response;
        responses.msg( i ).SerializeToString( &current_response );
        all_responses = all_responses + static_cast<string>( Integer32( current_response.size() ) ) + current_response;
    }

    client.write( first_response + all_requests + all_responses );
}
