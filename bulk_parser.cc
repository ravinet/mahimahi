/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include "int32.hh"
#include "bulk_parser.hh"
#include "http_record.pb.h"

using namespace std;

void BulkParser::parse( const string & response )
{
    string bulk_response = response;

    /* Request protobuf size */
    Integer32 req_size = static_cast<Integer32>( bulk_response.substr( 0,4 ) );
    bulk_response = bulk_response.substr( 4 );
    cout << "Request protobuf size: " << req_size << endl;

    /* Iterate through requests and print first lines */
    string request_proto = bulk_response.substr( 0, req_size );
    bulk_response = bulk_response.substr( req_size );
    MahimahiProtobufs::BulkMessage requests;
    requests.ParseFromString( request_proto );
    for ( int i = 0; i < requests.msg_size(); i++ ) {
        cout << requests.msg( i ).first_line() << endl;
    }

    /* Response protobuf size */
    Integer32 res_size = static_cast<Integer32>( bulk_response.substr( 0,4 ) );
    bulk_response = bulk_response.substr( 4 );
    cout << "Response protobuf size: " << res_size << endl;

    if ( bulk_response.size() > res_size ) { /* remove excess part of string if bulk response is smaller than 2^18 bytes */
        bulk_response = bulk_response.substr( 0, res_size );
    }

    /* Iterate throgh responses and print first lines */
    string response_proto = bulk_response.substr( 0, res_size );
    bulk_response = bulk_response.substr( res_size );
    MahimahiProtobufs::BulkMessage responses;
    responses.ParseFromString( response_proto );
    for ( int i = 0; i < responses.msg_size(); i++ ) {
        cout << responses.msg( i ).first_line() << endl;
    }

    /* Make sure the string is now empty */
    assert( bulk_response.size() == 0 );
}
