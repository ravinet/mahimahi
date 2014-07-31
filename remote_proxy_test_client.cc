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

int match_length( const string & first, const string & second )
{
    const auto max_match = min( first.size(), second.size() );
    for ( unsigned int i = first.find( "?" ); i < max_match; i++ ) {
        if ( first.at( i ) != second.at( i ) ) {
            return i;
        }
    }
    return max_match;
}

string strip_query( const string & request_line )
{
    const auto index = request_line.find( "?" );
    if ( index == string::npos ) {
        return request_line;
    } else {
        return request_line.substr( 0, index );
    }
}

/* matches client request with a request in bulk response and then return the corresponding response in bulk response */
MahimahiProtobufs::HTTPMessage find_response( MahimahiProtobufs::BulkMessage & requests,
                                              MahimahiProtobufs::BulkMessage & responses,
                                              MahimahiProtobufs::BulkRequest & client_request )
{
    pair< int, MahimahiProtobufs::HTTPMessage > possible_match = make_pair( 0, MahimahiProtobufs::HTTPMessage() );
    HTTPRequest new_request( client_request.request() );
    for ( int i = 0; i < requests.msg_size(); i++ ) {
        HTTPRequest current_request( requests.msg( i ) );
        if ( current_request.first_line() == new_request.first_line() ) { /* exact request line match...check host header */
            if ( current_request.get_header_value( "Host" ) == new_request.get_header_value( "Host" ) ) { /* host matches, send response */
                return responses.msg( i );
            }
        }
        /* request line isn't exact match...check if possible match */
        if ( strip_query( current_request.first_line() ) == strip_query( new_request.first_line() ) ) { /* up to "?" does match */
            if ( current_request.get_header_value( "Host" ) == new_request.get_header_value( "Host" ) ) { /* host matches too */
                int match_val = match_length( current_request.first_line(), new_request.first_line() );
                if (match_val > possible_match.first ) { /* this possible match is closer to client request */
                    possible_match = make_pair( match_val, responses.msg( i ) );
                }
            }
        }
    }
    if ( possible_match.first != 0 ) { /* we had a possible match */
        return possible_match.second;
    }
    throw Exception( "test_client", "Could not find matching request for: " + new_request.first_line() );
}

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

        bool parsed_requests = false;

        MahimahiProtobufs::BulkMessage requests;
        MahimahiProtobufs::BulkMessage responses;

        while ( not server.eof() ) {
            auto res = bulk_parser.parse( server.read() );
            if ( res.first ) { /* We read a complete bulk protobuf */
                if ( not parsed_requests ) {
                    requests.ParseFromString( res.second );
                    parsed_requests = true;
                } else {
                    responses.ParseFromString( res.second );
                }
            }
        }
        MahimahiProtobufs::HTTPMessage matched_response = find_response( requests, responses, bulk_request );
        cout << matched_response.first_line() << endl;
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
