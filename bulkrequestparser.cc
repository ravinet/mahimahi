/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <cassert>

#include "util.hh"
#include "exception.hh"
#include "file_descriptor.hh"
#include "int32.hh"
#include "length_value_parser.hh"
#include "http_record.pb.h"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " bulk_request_file " );
        }

        /* Open file */
        FileDescriptor bulk_request_file = SystemCall( "open", open( argv[ 0 ], O_RDONLY ) );
        string bulk_request( bulk_request_file.read() );

        /* Parse it using LengthValueParser */
        LengthValueParser bulk_request_parser;
        auto res = bulk_request_parser.parse( bulk_request );
        assert( res.first );

        /* Print incoming request url (to be given to phantomjs) */
        MahimahiProtobufs::BulkRequest request;
        request.ParseFromString( res.second );
        string scheme = (request.scheme() == MahimahiProtobufs::BulkRequest_Scheme_HTTPS
                        ? "https"
                        : "http" );
        MahimahiProtobufs::HTTPMessage request_message = request.request();
        string hostname;
        for ( int i = 0; i < request_message.header_size(); i++ ) {
            if ( request_message.header( i ).key() == "Host" ) {
                hostname = request_message.header( i ).value();
            }
        }
        string url = scheme + "://" + hostname;
        cout << url << endl;

        /* Print request body itself */
        cout << request_message.DebugString() << endl;
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
