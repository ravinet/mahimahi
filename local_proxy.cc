/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <utility>

#include "socket.hh"
#include "event_loop.hh"
#include "length_value_parser.hh"
#include "http_record.pb.h"
#include "http_header.hh"
#include "int32.hh"
#include "http_request.hh"
#include "http_request_parser.hh"
#include "poller.hh"

using namespace std;
using namespace PollerShortNames;

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
        if ( strip_query( current_request.first_line() ) == strip_query( new_request.first_line() ) ) { /* up to ? matches */
            if ( current_request.get_header_value( "Host" ) == new_request.get_header_value( "Host" ) ) { /* host header matches */
                if ( current_request.first_line() == new_request.first_line() ) { /* exact match */
                    return responses.msg( i );
                }
                /* possible match, not exact match */
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

void handle_client( Socket && client, const Address & remote_proxy, const string & scheme )
{
        Socket server( TCP );
        server.connect( remote_proxy );

        HTTPRequestParser request_parser;

        LengthValueParser bulk_parser;

        bool parsed_requests = false;

        MahimahiProtobufs::BulkMessage requests;
        MahimahiProtobufs::BulkMessage responses;

        MahimahiProtobufs::BulkRequest bulk_request;

        Poller poller;

        poller.add_action( Poller::Action( client, Direction::In,
                                           [&] () {
                                               string buffer = client.read();
                                               request_parser.parse( buffer );
                                               return ResultType::Continue;
                                           },
                                           [&] () { return true; } ) );

        poller.add_action( Poller::Action( server, Direction::Out,
                                           [&] () {
                                               bulk_request.set_scheme( scheme == "https" ? MahimahiProtobufs::BulkRequest_Scheme_HTTPS :
                                                                                            MahimahiProtobufs::BulkRequest_Scheme_HTTP );
                                               bulk_request.mutable_request()->CopyFrom( request_parser.front().toprotobuf() );
                                               request_parser.pop();
                                               string request_proto;
                                               bulk_request.SerializeToString( &request_proto );

                                               /* Make request string to send (prepended with size) */
                                               string request = static_cast<string>( Integer32( request_proto.size() ) ) + request_proto;

                                               server.write( request );

                                               return ResultType::Continue;
                                           },
                                           [&] () { return not request_parser.empty(); } ) );

        poller.add_action( Poller::Action( server, Direction::In,
                                           [&] () {
                                               string buffer = server.read();
                                               auto res = bulk_parser.parse( buffer );
                                               if ( res.first ) { /* we have read a complete bulk protobuf */
                                                   if ( not parsed_requests ) { /* it is the requests */
                                                       requests.ParseFromString( res.second );
                                                       parsed_requests = true;
                                                   } else { /* it is the responses */
                                                       responses.ParseFromString( res.second );
                                                       MahimahiProtobufs::HTTPMessage matched_response = find_response( requests, responses, bulk_request );
                                                       client.write( matched_response.first_line() );
                                                   }
                                               }
                                               return ResultType::Continue;
                                           },
                                           [&] () { return not server.eof(); } ) );

        while ( true ) {
            if ( poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
                return;
            }
        }
}

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 6 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " local_proxy_ip local_proxy_port remote_proxy_ip remote_proxy_port scheme" );
        }

        Address remote_proxy( string( argv[ 3 ] ), argv[ 4 ] );

        Socket listener_socket( TCP );
        listener_socket.bind( Address( argv[ 1 ], argv[ 2 ] ) );
        listener_socket.listen();

        EventLoop event_loop;

        /* listen for new clients */
        event_loop.add_simple_input_handler( listener_socket,
                                             [&] () {
                                                 Socket client = listener_socket.accept();
                                                 event_loop.add_child_process( ChildProcess( "new_client", [&] () {
                                                         handle_client( move( client ), remote_proxy, string( argv[ 5 ] ) );
                                                         return EXIT_SUCCESS;
                                                     } ), false );
                                                 return ResultType::Continue;
                                             } );

        return event_loop.loop();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
