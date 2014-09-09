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
#include "length_value_parser.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " bulk_response_file " );
        }

        /* Open file */
        FileDescriptor bulk_response_file = SystemCall( "open", open( argv[ 1 ], O_RDONLY ) );
        string bulk_response( bulk_response_file.read() );

        /* Parse it using LengthValueParser */
        LengthValueParser bulk_response_parser;
        bool saw_first_response = false;
        bool parsed_requests = false;
        auto res = bulk_response_parser.parse( bulk_response );
        while ( res.first ) {
            if ( not saw_first_response ) { /* this is the first response */
                MahimahiProtobufs::HTTPMessage first_response;
                first_response.ParseFromString( res.second );
                cout << "Response for first request" << endl
                     << first_response.DebugString() << endl;
                saw_first_response = true;
            } else if ( not parsed_requests ) { /* it is the requests */
                MahimahiProtobufs::BulkMessage requests_list;
                requests_list.ParseFromString( res.second );
                cout << "Number of requests: " << requests_list.msg_size() << endl
                     << requests_list.DebugString() << endl;
                parsed_requests = true;
            } else { /* it is a response */
                MahimahiProtobufs::HTTPMessage current_response;
                current_response.ParseFromString( res.second );
                cout << "Current Response" << endl
                     << current_response.DebugString() << endl;
            }
            res = bulk_response_parser.parse( "" );
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
