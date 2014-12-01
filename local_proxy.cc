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
#include "timestamp.hh"

using namespace std;
using namespace PollerShortNames;

LocalProxy::LocalProxy( const Address & listener_addr, const Address & remote_proxy )
    : listener_socket_( TCP ),
      remote_proxy_addr_( remote_proxy ),
      archive(),
      sent_requests()
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

int LocalProxy::add_bulk_requests( const string & bulk_requests, vector< pair< int, int > > & request_positions )
{
    MahimahiProtobufs::BulkMessage requests;
    requests.ParseFromString( bulk_requests );
    /* add requests to archive */
    for ( int i = 0; i < requests.msg_size(); i++ ) {
        /* Don't check freshness since these are newer than whatever is in archive */
        auto find_result = archive.find_request( requests.msg( i ), false, false );
        if ( find_result.first == false ) { /* request not already in archive */
            auto pos = archive.add_request( requests.msg( i ) );
            request_positions.emplace_back( make_pair( i, pos ) );
        }
    }
    return requests.msg_size();
}

void LocalProxy::handle_response( const string & res, const vector< pair< int, int > > & request_positions, int & response_counter )
{
    MahimahiProtobufs::HTTPMessage single_response;
    single_response.ParseFromString( res );
    for ( unsigned int j = 0; j < request_positions.size(); j++ ) {
        if ( response_counter == request_positions.at( j ).first ) { /* we want to add this response */
            archive.add_response( single_response, request_positions.at( j ).second );
        }
    }
    response_counter++;
}

template <class SocketType>
void LocalProxy::handle_new_requests( Poller & client_poller, HTTPRequestParser & request_parser, SocketType && client )
{
    bool cannot_handle = false;
    client_poller.poll( 0 );
    while ( ( not request_parser.empty() ) and ( not cannot_handle ) ) {
        auto new_req_status = archive.find_request( request_parser.front().toprotobuf(), false );
        if ( new_req_status.first and ( new_req_status.second != "" ) ) { /* we have new request on same thread */
            //cout << "WROTE RESPONSE FOR: " << request_parser.front().first_line() << " AT: " << timestamp() << endl;
            client.write( new_req_status.second );
            request_parser.pop();
        } else {
            cannot_handle = true;
        }
    }
}

/* function blocks until it can send a response back to the client for the given request */
template <class SocketType1, class SocketType2>
bool LocalProxy::get_response( const HTTPRequest & new_request, const string & scheme, SocketType1 && server, bool & already_connected, SocketType2 && client, HTTPRequestParser & request_parser )
{
    //cout << "INCOMING REQUEST: " << new_request.first_line() << "AT: " << timestamp() << endl;

    /* check if request and response are in the archive */
    auto to_send = archive.find_request( new_request.toprotobuf(), false );
    if ( to_send.first ) {
        while ( to_send.second == "" ) { /* response is pending */
            archive.wait();
            to_send = archive.find_request( new_request.toprotobuf(), false );
        }
        //cout << "WROTE RESPONSE FOR: " << new_request.first_line() << " AT: " << timestamp() << endl;
        client.write( to_send.second );
        request_parser.pop();
        return false;
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

    bool first_response = true;

    bool finished_bulk = false;

    int response_counter = 0;

    int total_requests = 0;

    Poller client_poller;

    client_poller.add_action( Poller::Action( client, Direction::In,
                                       [&] () {
                                           string buffer = client.read();
                                           request_parser.parse( buffer );
                                           return ResultType::Continue;
                                       },
                                       [&] () { return not server.eof(); } ) );

    server_poller.add_action( Poller::Action( server, Direction::Out,
                                       [&] () {
                                           server.write( make_bulk_request( new_request , scheme ) );
                                           sent_request = true;
                                           auto path_start = new_request.first_line().find( "/" );
                                           auto path_end = new_request.first_line().find( " ", path_start );
                                           string req_sent = new_request.get_header_value( "Host" ) + new_request.first_line().substr( path_start, path_end - path_start );
                                           sent_requests.emplace_back( req_sent );
                                           return ResultType::Continue;
                                       },
                                       [&] () { return not sent_request; } ) );

    server_poller.add_action( Poller::Action( server, Direction::In,
                                       [&] () {
                                           string buffer = server.read();
                                           auto res = bulk_parser.parse( buffer );
                                           while ( res.first ) {
                                               if ( not parsed_requests ) { /* it is the requests */
                                                   parsed_requests = true;
                                                   total_requests = add_bulk_requests( res.second, request_positions );
                                               } else { /* it is a response */
                                                   //cout << "RECEIVED A RESPONSE AT: " << timestamp() << endl;
                                                   if ( first_response ) { /* this is the first response, send back to client */
                                                       MahimahiProtobufs::HTTPMessage response_to_send;
                                                       response_to_send.ParseFromString( res.second );
                                                       HTTPResponse first_res( response_to_send );
                                                       client.write( first_res.str() );
                                                       request_parser.pop();
                                                       first_response = false;
                                                   }
                                                   handle_response( res.second, request_positions, response_counter );
                                                   handle_new_requests( client_poller, request_parser, move( client ) );
                                               }
                                               res = bulk_parser.parse( "" );
                                           }
                                           if ( total_requests == response_counter && total_requests != 0 ) {
                                               //cout << "FINISHED ADDING BULK FOR: " << new_request.first_line() << " WITH # RES: " << total_requests << " AT: " << timestamp() << endl;
                                               finished_bulk = true;
                                           }
                                           return ResultType::Continue;
                                       },
                                       [&] () { return sent_request; } ) );

    while ( true ) {
        if ( server_poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
            return true;
        }
        if ( finished_bulk ) {
            return false;
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

    poller.add_action( Poller::Action( client, Direction::In,
                                       [&] () {
                                           string buffer = client.read();
                                           request_parser.parse( buffer );
                                           while ( not request_parser.empty() ) { /* we have a complete request */
                                               HTTPRequest new_req( request_parser.front() );
                                               bool status = get_response( new_req, scheme, move( server ), already_connected, move( client ), request_parser );
                                               if ( status ) { /* we should handle response */
                                                   return ResultType::Exit;
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

void LocalProxy::print_sent_requests( void )
{
    for ( uint i = 0; i < sent_requests.size(); i++ ) {
        cout << "Sent: " << sent_requests.at( i ) << endl;
    }
    cout << "Total requests sent: " << sent_requests.size() << endl;
}
