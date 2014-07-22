/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include "int32.hh"
#include "http_memory_store.hh"
#include "http_record.pb.h"

using namespace std;

HTTPMemoryStore::HTTPMemoryStore()
    : mutex_(),
      bulk_requests(),
      bulk_responses()
{}

void HTTPMemoryStore::save( const HTTPResponse & response, const Address & server_address )
{
    unique_lock<mutex> ul( mutex_ );
    cout << "Request to: " << server_address.ip() << endl; 
    bulk_requests.emplace_back( response.request().toprotobuf() );
    bulk_responses.emplace_back( response.toprotobuf() );
}

string HTTPMemoryStore::serialize_to_string( void )
{
    unique_lock<mutex> ul( mutex_ );
    /* make sure there is a 1 to 1 matching of requests and responses */
    assert( bulk_requests.size() == bulk_responses.size() );

    string bulk_message;

    /* bulk response starts with total number of request/response pairs */
    bulk_message.append( static_cast<string>( Integer32( bulk_requests.size() ) ) );

    /* append each request to bulk response (list request size before each request) */
    for ( unsigned int i = 0; i < bulk_requests.size(); i++ ) {
        string current_request;
        bulk_requests.at( i ).SerializeToString( &current_request );
        /* must make this only be a certain number of bytes so we can parse? */
        bulk_message.append( static_cast<string>( Integer32( current_request.size() ) ) );
        bulk_message.append( current_request );
    }

    /* append each response to bulk response (list response size before each response) */
    for ( unsigned int i = 0; i < bulk_responses.size(); i++ ) {
        string current_response;
        bulk_responses.at( i ).SerializeToString( &current_response );
        /* must make this only be a certain number of bytes so we can parse? */
        bulk_message.append( static_cast<string>( Integer32( current_response.size() ) ) );
        bulk_message.append( current_response );
    }

    return bulk_message;
}
