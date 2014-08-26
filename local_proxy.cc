/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <thread>

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

string make_bulk_request( const HTTPRequest & request, const string & scheme )
{
    MahimahiProtobufs::BulkRequest bulk_request;

    bulk_request.set_scheme( scheme == "https" ? MahimahiProtobufs::BulkRequest_Scheme_HTTPS :
                                                 MahimahiProtobufs::BulkRequest_Scheme_HTTP );
    bulk_request.mutable_request()->CopyFrom( request.toprotobuf() );

    string request_proto;
    bulk_request.SerializeToString( &request_proto );

    /* Return request string to send (prepended with size) */
    return( static_cast<string>( Integer32( request_proto.size() ) ) + request_proto );
}

/* function blocks until it can send a response back to the client for the given request */
template <class SocketType1, class SocketType2>
pair< bool, string> LocalProxy::get_response( const HTTPRequest & new_request, const string & scheme, SocketType1 && server, bool & already_connected, SocketType2 && client )
{
    /* first check if request is POST and if so, respond that we can't find the response */
    string type = new_request.str().substr( 0, 4 );
    if ( type == "POST" ) { /* POST request so send back can't find */
        client.write( could_not_find );
        return make_pair( true, could_not_find );
    }

    /* check if request and response are in the archive */
    auto to_send = archive.find_request( new_request.toprotobuf(), false );
    if ( to_send.first == true ) {
        client.write( to_send.second );
        return make_pair( true, to_send.second );
    }

    if ( not already_connected ) {
        server.connect( remote_proxy_addr_ );
        already_connected = true;
    }

    /* not in archive, so send to remote proxy and wait for response */
    Poller server_poller;

    LengthValueParser bulk_parser;

    vector< pair< int, int > > request_positions;

    bool parsed_requests = false;

    bool sent_request = false;

    bool first_response = false;

    bool finished_bulk = false;

    server_poller.add_action( Poller::Action( server, Direction::Out,
                                       [&] () {
                                           server.write( make_bulk_request( new_request , scheme ) );
                                           sent_request = true;
                                           return ResultType::Continue;
                                       },
                                       [&] () { return not sent_request; } ) );

    server_poller.add_action( Poller::Action( server, Direction::In,
                                       [&] () {
                                           string buffer = server.read();
                                           auto res = bulk_parser.parse( buffer );
                                           if ( res.first ) { /* we have read a complete bulk protobuf */
                                               if ( not first_response ) { /* this is the first response, send back to client */
                                                   MahimahiProtobufs::HTTPMessage response_to_send;
                                                   response_to_send.ParseFromString( res.second );
                                                   HTTPResponse first_res( response_to_send );
                                                   client.write( first_res.str() );
                                                   first_response = true;
                                               } else if ( not parsed_requests ) { /* it is the requests */
                                                   MahimahiProtobufs::BulkMessage requests;
                                                   requests.ParseFromString( res.second );
                                                   parsed_requests = true;
                                                   /* add requests to archive */
                                                   for ( int i = 0; i < requests.msg_size(); i++ ) {
                                                       /* Don't check freshness since these are newer than whatever is in archive */
                                                       auto find_result = archive.find_request( requests.msg( i ), false, false );
                                                       if ( find_result.first == false ) { /* request not already in archive */
                                                           auto pos = archive.add_request( requests.msg( i ) );
                                                           request_positions.emplace_back( make_pair( i, pos ) );
                                                       }
                                                   }
                                               } else { /* it is the responses */
                                                   MahimahiProtobufs::BulkMessage responses;
                                                   responses.ParseFromString( res.second );
                                                   for ( unsigned int j = 0; j < request_positions.size(); j++ ) {
                                                       archive.add_response( responses.msg( request_positions.at( j ).first ), request_positions.at( j ).second );
                                                   }
                                                   finished_bulk = true;
                                               }
                                           }
                                           return ResultType::Continue;
                                       },
                                       [&] () { return sent_request; } ) );

    while ( true ) {
        if ( server_poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
            return make_pair( true, "" );
        }
        if ( finished_bulk ) {
            return make_pair( false, "" );
        }
    }
}

template <class SocketType>
void LocalProxy::handle_client( SocketType && client, const string & scheme )
{
    HTTPRequestParser request_parser;

    Poller poller;

    Socket server( TCP );
    bool already_connected = false;

    /* Currently we can only have one response at a time anyway (should replace with bytestreamqueue eventually) */
    string current_response;

    poller.add_action( Poller::Action( client, Direction::In,
                                       [&] () {
                                           string buffer = client.read();
                                           request_parser.parse( buffer );
                                           if ( not request_parser.empty() ) { /* we have a complete request */
                                               auto status = get_response( request_parser.front(), scheme, move( server ), already_connected, move( client ) );
                                               request_parser.pop();
                                               if ( status.first == true ) { /* we should handle response */
                                                   if ( current_response == "" ) {
                                                       return ResultType::Exit;
                                                   }
                                                   //current_response = status.second;
                                               }
                                           }
                                           return ResultType::Continue;
                                       },
                                       [&] () { return ( ( not server.eof() ) and ( current_response.empty() ) ); } ) );

/*    poller.add_action( Poller::Action( client, Direction::Out,
                                       [&] () {
                                           client.write( current_response );
                                           current_response.clear();
                                           return ResultType::Continue;
                                       },
                                       [&] () { return not current_response.empty(); } ) );
*/

    while ( true ) {
        if ( poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
            return;
        }
    }
}

void LocalProxy::listen( void )
{

    thread newthread( [&] ( Socket client ) {
            try {
                string scheme = "http";
                if ( client.original_dest().port() == 443 ) {
                    scheme = "https";
                    SSLContext server_context( SERVER );
                    SecureSocket tls_client( server_context.new_secure_socket( move( client ) ) );
                    tls_client.accept();
                    handle_client( move( tls_client ), scheme );
                    return;
                }
                handle_client( move( client ), scheme );
                return;
            } catch ( const Exception & e ) {
                e.perror();
            }
    }, listener_socket_.accept() );

    /* don't wait around for the reply */
    newthread.detach();
}

void LocalProxy::register_handlers( EventLoop & event_loop )
{
    event_loop.add_simple_input_handler( tcp_listener(),
                                         [&] () {
                                             listen();
                                             return ResultType::Continue;
                                         } );
}
