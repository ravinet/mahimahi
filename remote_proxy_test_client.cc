/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <utility>

#include "socket.hh"
#include "event_loop.hh"
#include "length_value_parser.hh"
#include "http_record.pb.h"
#include "http_header.hh"
#include "int32.hh"
#include "http_request.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 6 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " remote_proxy_addr remote_proxy_port scheme hostname path" );
        }

        /* Connect to remote_proxy before anything else */
        Socket server( TCP );
        server.connect( Address( argv[ 1 ], argv[ 2 ] ) );

        /* Read cmd line args for the website you are trying to reach */
        string scheme( argv[ 3 ] );
        string hostname( argv[ 4 ] );
        string path( argv[ 5 ] );

        /* parse url into scheme and HTTPMessage request */
        if ( path.at( 0 ) != '/' ) {
            throw Exception( string( argv[ 0 ] ), "path must begin with /" );
        }

        /* Construct bulk request */
        MahimahiProtobufs::BulkRequest bulk_request;
        bulk_request.set_scheme( scheme == "https"
                                           ? MahimahiProtobufs::BulkRequest_Scheme_HTTPS
                                           : MahimahiProtobufs::BulkRequest_Scheme_HTTP );
        MahimahiProtobufs::HTTPMessage request_message;
        request_message.set_first_line( "GET " + path + " HTTP/1.1" );
        HTTPHeader request_header( "Host: " + hostname );
        request_message.add_header()->CopyFrom( request_header.toprotobuf() );
        bulk_request.mutable_request()->CopyFrom( request_message );
        string request_proto;
        bulk_request.SerializeToString( &request_proto );

        /* Make request string to send (prepended with size) */
        string request = static_cast<string>( Integer32( request_proto.size() ) ) + request_proto;

        /* Write to remote_proxy */
        server.write( request );

        /* Parse responses from server using a LengthValueParser as they come in */
        LengthValueParser bulk_parser;
        while ( not server.eof() ) {
            auto res = bulk_parser.parse( server.read() );
            if ( res.first ) {
                /* We read a complete bulk protobuf */
                cout << "Found response to client request" << endl;
                cout << res.second << endl;
                break;
            }
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
