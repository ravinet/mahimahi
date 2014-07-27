/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <cassert>

#include "util.hh"
#include "exception.hh"
#include "file_descriptor.hh"
#include "int32.hh"
#include "http_record.pb.h"

using namespace std;

int main()
{
    try {
        FileDescriptor bulkreply = SystemCall( "open", open( "bulk_reply.txt", O_RDONLY ) );

        string bulk_response( bulkreply.read() );

        /* The first 4-byte number represents the total number of request/response pairs */
        Integer32 pairs = static_cast<Integer32>( bulk_response.substr( 0,4 ) );
        bulk_response = bulk_response.substr( 4 );
        cout << "Request/Response pairs: " << pairs << endl;

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

        /* The next 4-byte number represents the total number of request/response pairs */
        bulk_response = bulk_response.substr( 4 );

        /* Response protobuf size */
        Integer32 res_size = static_cast<Integer32>( bulk_response.substr( 0,4 ) );
        bulk_response = bulk_response.substr( 4 );
        cout << "Response protobuf size: " << res_size << endl;

        /* Iterate through responses and print first lines */
        if ( bulk_response.size() > res_size ) { /* remove excess part of string if bulk response is smaller than 2^18 bytes */
           bulk_response = bulk_response.substr( 0, res_size );
        } 
        while ( bulk_response.size() < res_size ) { /* if bulk response larger than 2^18 bytes, read until we have all of it */
            string bulk2( bulkreply.read() );
            bulk_response = bulk_response + bulk2;
        }
        string response_proto = bulk_response.substr( 0, res_size );
        bulk_response = bulk_response.substr( res_size );
        MahimahiProtobufs::BulkMessage responses;
        responses.ParseFromString( response_proto );
        for ( int i = 0; i < responses.msg_size(); i++ ) {
            cout << responses.msg( i ).first_line() << endl;
        }

        /* Make sure the string is now empty */
        assert( bulk_response.size() == 0 );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
