/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include "int32.hh"
#include "bulk_parser.hh"
#include "http_record.pb.h"

using namespace std;

void BulkParser::parse( const string & response )
{
    buffer.append( response );

    switch ( state_ ) {
    case REQ_SIZE: {
        if ( buffer.size() < 4 ) { /* Don't have enough for request protobuf size */
            return;
        }
        request_proto_size = static_cast<Integer32>( buffer.substr( 0,4 ) );
        buffer = buffer.substr( 4 );
        cout << "Request protobuf size: " << request_proto_size << endl;
        state_ = REQ;
    }

    case REQ: {
        if ( buffer.size() < request_proto_size ) { /* Don't have enough for request protobuf */
            return;
        }
        /* Iterate through requests and print first lines */
        MahimahiProtobufs::BulkMessage requests;
        requests.ParseFromString( buffer.substr( 0, request_proto_size ) );
        buffer = buffer.substr( request_proto_size );
        for ( int i = 0; i < requests.msg_size(); i++ ) {
            cout << requests.msg( i ).first_line() << endl;
        }
        state_ = RES_SIZE;
    }

    case RES_SIZE: {
        if ( buffer.size() < 4 ) { /* Don't have enough for request protobuf size */
            return;
        }
        response_proto_size  = static_cast<Integer32>( buffer.substr( 0,4 ) );
        buffer = buffer.substr( 4 );
        cout << "Response protobuf size: " << response_proto_size << endl;
        state_ = RES;
    }

    case RES: {
        if ( buffer.size() < response_proto_size ) { /* Don't have enough for response protobuf */
            return;
        }
        if ( buffer.size() > response_proto_size ) { /* remove excess part of string if bulk response is smaller than 2^18 bytes */
            buffer = buffer.substr( 0, response_proto_size );
        }

        /* Iterate throgh responses and print first lines */
        //string response_proto = bulk_response.substr( 0, res_size );
        //bulk_response = bulk_response.substr( res_size );
        MahimahiProtobufs::BulkMessage responses;
        responses.ParseFromString( buffer.substr( 0, response_proto_size ) );
        buffer = buffer.substr( 0, response_proto_size );
        for ( int i = 0; i < responses.msg_size(); i++ ) {
            cout << responses.msg( i ).first_line() << endl;
        }

        /* Make sure the string is now empty */
        assert( buffer.size() == 0 );
    }
    }
}
