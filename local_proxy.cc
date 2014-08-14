/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include "address.hh"
#include "system_runner.hh"
#include "poller.hh"
#include "http_request_parser.hh"
#include "event_loop.hh"
#include "local_proxy.hh"
#include "length_value_parser.hh"
#include "http_request.hh"
#include "http_header.hh"
#include "http_response.hh"

using namespace std;
using namespace PollerShortNames;

LocalProxy::LocalProxy( const Address & listener_addr, const Address & remote_proxy )
    : listener_socket_( TCP ),
      remote_proxy_addr_( remote_proxy ),
      archive()
{
    listener_socket_.bind( listener_addr );
    listener_socket_.listen();
}

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
    MahimahiProtobufs::HTTPMessage ret;
    ret.set_first_line( "HTTP/1.1 200 OK" );
    HTTPHeader header1( "Content-Type: Text/html" );
    HTTPHeader header2( "Connection: close");
    HTTPHeader header3( "Content-Length: 24" );
    ret.set_body( "COULD NOT FIND AN OBJECT" );
    ret.add_header()->CopyFrom( header1.toprotobuf() );
    ret.add_header()->CopyFrom( header2.toprotobuf() );
    ret.add_header()->CopyFrom( header3.toprotobuf() );
    return ret;
}

template <class SocketType>
void LocalProxy::handle_client( SocketType && client, const string & scheme )
{
    Socket server( TCP );
    server.connect( remote_proxy_addr_ );

    HTTPRequestParser request_parser;

    LengthValueParser bulk_parser;

    vector< pair< int, int > > request_positions;

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
                                           /* don't send POST requests to the remote proxy */
                                           string type = request_parser.front().str().substr( 0, 4 );
                                           if ( type == "POST" ) { /* POST request so send back can't find */
                                               server.write( "" );
                                               client.write( "HTTP/1.1 200 OK\r\nContent-Type: Text/html\r\nConnection: close\r\nContent-Length: 24\r\n\r\nCOULD NOT FIND AN OBJECT" );
                                               request_parser.pop();
                                               return ResultType::Continue;
                                           }
                                           auto tosend = archive.find_request( request_parser.front().toprotobuf() );
                                           if ( tosend.first == true ) {
                                               server.write( "" );
                                               client.write( tosend.second );
                                               request_parser.pop();
                                               return ResultType::Continue;
                                           }
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
                                                   /* add requests to archive */
                                                   for ( int i = 0; i < requests.msg_size(); i++ ) {
                                                       auto pos = archive.add_request( requests.msg( i ) );
                                                       if ( pos >= 0 ) {
                                                           request_positions.emplace_back( make_pair( i, pos ) );
                                                       }
                                                   }
                                               } else { /* it is the responses */
                                                   responses.ParseFromString( res.second );
                                                   for ( int j = 0; j < request_positions.size(); j++ ) {
                                                       archive.add_response( responses.msg( request_positions.at( j ).first ), request_positions.at( j ).second );
                                                   }
                                                   request_positions.clear();
                                                   auto tosend = archive.find_request( bulk_request.request() );
                                                   if ( tosend.first == true ) {
                                                       client.write( tosend.second );
                                                   } else {
                                                       client.write( "HTTP/1.1 200 OK\r\nContent-Type: Text/html\r\nConnection: close\r\nContent-Length: 24\r\n\r\nCOULD NOT FIND AN OBJECT" );
                                                   }
                                                   archive.print( bulk_request.request() );
                                                   parsed_requests = false;
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

void LocalProxy::listen( EventLoop & event_loop )
{
    /* listen for new clients */
    event_loop.add_simple_input_handler( listener_socket_,
                                         [&] () {
                                             Socket client = listener_socket_.accept();
                                             event_loop.add_child_process( ChildProcess( "new_client", [&] () {
                                                     string scheme = "http";
                                                     if ( client.original_dest().port() == 443 ) {
                                                         scheme = "https";
                                                         SSLContext server_context( SERVER );
                                                         SecureSocket tls_client( server_context.new_secure_socket( move( client ) ) );
                                                         tls_client.accept();
                                                         handle_client( move( tls_client ), scheme );
                                                         return EXIT_SUCCESS;
                                                     }
                                                     handle_client( move( client ), scheme );
                                                     return EXIT_SUCCESS;
                                                 } ), false );
                                             return ResultType::Continue;
                                         } );
}
