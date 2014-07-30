/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "socket.hh"
#include "event_loop.hh"
#include "length_value_parser.hh"
#include "http_record.pb.h"
#include "http_header.hh"
#include "int32.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 4 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " host port url" );
        }

        Socket server( TCP );
        server.connect( Address( argv[ 1 ], argv[ 2 ] ) );

        /* parse url into scheme and HTTPMessage request */
        string url( argv[ 3 ] );
        string scheme = url.substr( 0, url.find( ":" ) );
        string hostname = url.substr( url.find( ":" ) + 3 );

        /* Construct bulk request */
        MahimahiProtobufs::BulkRequest bulk_request;
        bulk_request.set_scheme( scheme == "https" ? MahimahiProtobufs::BulkRequest_Scheme_HTTPS : MahimahiProtobufs::BulkRequest_Scheme_HTTP );
        MahimahiProtobufs::HTTPMessage request_message;
        request_message.set_first_line( "GET / HTTP/1.1" );
        HTTPHeader request_header( "Host: " + hostname );
        request_message.add_header()->CopyFrom( request_header.toprotobuf() );
        bulk_request.mutable_request()->CopyFrom( request_message );
        string request_proto;
        bulk_request.SerializeToString( &request_proto );

        /* Make request string to send (prepended with size) */
        string request = static_cast<string>( Integer32( request_proto.size() ) ) + request_proto;

        server.write( request );

        LengthValueParser bulk_parser;

        while ( not server.eof() ) {
            auto res = bulk_parser.parse( server.read() );
            if ( res.first ) { /* We read a complete bulk protobuf */
                MahimahiProtobufs::BulkMessage message;
                message.ParseFromString( res.second );
                /* Print first line of each request or response in protobuf */
                for ( int i = 0; i < message.msg_size(); i++ ) {
                    cout << message.msg( i ).first_line() << endl;
                }
            }
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
