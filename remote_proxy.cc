/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <unistd.h>
#include <string>

#include "util.hh"
#include "process_recorder.hh"
#include "process_recorder.cc"
#include "exception.hh"
#include "event_loop.hh"
#include "socket.hh"
#include "address.hh"
#include "poller.hh"
#include "system_runner.hh"
#include "config.h"
#include "phantomjs_configuration.hh"
#include "http_memory_store.hh"
#include "length_value_parser.hh"
#include "http_header.hh"
#include "http_message.hh"

using namespace std;
using namespace PollerShortNames;

string remove_single_quotes( string current_header )
{
    /* change all single quotes to \' */
    size_t loc = current_header.find("'");
    while ( loc != string::npos ) { /* we have a ' */
        if ( current_header[loc] == '\'' and current_header[loc-1] != '\\') { /* must replace */
            current_header.replace(loc, 1, "\\\'");
        }
        loc = current_header.find("'", loc+1);
    }
    return current_header;
}

string make_phantomjs_script( const MahimahiProtobufs::BulkRequest & incoming_request )
{
    /* Obtain scheme from BulkRequest protobuf */
    string scheme = incoming_request.scheme() == MahimahiProtobufs::BulkRequest_Scheme_HTTPS
                    ? "https" : "http";
    /* Obtain hostname from http request within BulkRequest */
    const HTTPRequest curr_request( incoming_request.request() );
    string hostname = curr_request.get_header_value( "Host" );

    /* Obtain path from http request within BulkRequest */
    auto path_start = curr_request.first_line().find( "/" );
    auto path_end = curr_request.first_line().find( " ", path_start );
    string path = curr_request.first_line().substr( path_start, path_end - path_start );

    /* Now concatenate the components into one URL */
    string url = scheme + "://" + hostname + path;

    /* Populate HTTP headers as per incoming_request */
    string custom_headers = "page.customHeaders = {";
    string user_agent_header = "page.settings.userAgent = ['";
    for ( const auto &x : incoming_request.request().header() ) {
        HTTPHeader http_header( x );
        if ( HTTPMessage::equivalent_strings( http_header.key(), "User-Agent" ) ) {
            user_agent_header += http_header.value();
        } else {
            if ( ( not HTTPMessage::equivalent_strings( http_header.key(), "Content-Length" ) ) and
                   ( not HTTPMessage::equivalent_strings( http_header.key(), "Accept-Encoding" ) ) ) {
                custom_headers += "'" + remove_single_quotes( http_header.key() ) + "' : '" + remove_single_quotes( http_header.value() ) + "',";
            }

        }
    }
    user_agent_header.append( "'];\n" );
    custom_headers.append( "};\n" );

    string data = incoming_request.request().body();

    if ( curr_request.first_line().find( "POST" ) != string::npos ) {
        //cout << ( "data = '" + data + "';\nurl = '" + url + "';\n" + phantomjs_setup + user_agent_header + custom_headers + phantomjs_load_post ) << endl;
        return( "data = '" + data + "';\nurl = '" + url + "';\n" + phantomjs_setup + user_agent_header + custom_headers + phantomjs_load_post );
    } else {
        //cout << ( "url = '" + url + "';\n" + phantomjs_setup + user_agent_header + custom_headers + phantomjs_load ) << endl;
        return( "url = '" + url + "';\n" + phantomjs_setup + user_agent_header + custom_headers + phantomjs_load );
    }
}

void handle_client( Socket && client, const int & veth_counter )
{
    try {
        /* LengthValueParser for bulk requests coming in from local_proxy? */
        LengthValueParser bulk_request_parser;

        Poller poller;

        /* Setup a process recorder with a HTTPProxy target and a
           and save requests and responses from the HTTPProxy to a
           HTTPMemoryStore */
        ProcessRecorder<HTTPProxy<HTTPMemoryStore>> process_recorder;

        /* Have we finished parsing the bulk request from local_proxy? */
        bool request_ready = false;

        MahimahiProtobufs::BulkRequest incoming_request;

        poller.add_action( Poller::Action( client, Direction::In,
                                           [&] () {
                                               string buffer = client.read();
                                               auto res = bulk_request_parser.parse( buffer );
                                               if ( res.first ) { /* request protobuf ready */
                                                   incoming_request.ParseFromString( res.second );
                                                   request_ready = true;
                                               }
                                               return ResultType::Continue;
                                           } ) );

        poller.add_action( Poller::Action( client, Direction::Out,
                                           [&] () {
                                               /* Pass the incoming_request to phantomjs within a ProcessRecorder */
                                               /* Block until it's done recording */
                                               process_recorder.record_process( []( FileDescriptor & parent_channel ) {
                                                                                SystemCall( "dup2", dup2( parent_channel.num(), STDIN_FILENO ) );
                                                                                return ezexec( { PHANTOMJS, "--ignore-ssl-errors=true",
                                                                                                 "--ssl-protocol=TLSv1", "/dev/stdin" } );
                                                                                },
                                                                                move( client ),
                                                                                veth_counter,
                                                                                incoming_request,
                                                                                make_phantomjs_script( incoming_request ) );
                                               /* Write a null string to prevent poller from throwing an exception */
                                               client.write( "" );
                                               return Result::Type::Exit;
                                           },
                                           [&] () { return request_ready; } ) );
        while ( true ) {
            if ( poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
                return;
            }
        }
    } catch ( const Exception & e ) {
        e.perror();
        return;
    }
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " port" );
        }

        /* Use this as part of veth interface name because pid is same for different clients */
        int veth_counter = 0;

        Socket listener_socket( TCP );
        listener_socket.bind( Address( "0", argv[ 1 ] ) );
        listener_socket.listen();

        EventLoop event_loop;

        /* listen for new clients */
        event_loop.add_simple_input_handler( listener_socket,
                                             [&] () {
                                                 Socket client = listener_socket.accept();
                                                 event_loop.add_child_process( ChildProcess( "new_client", [&] () {
                                                         handle_client( move( client ), veth_counter );
                                                         return EXIT_SUCCESS;
                                                     } ), false );
                                                 veth_counter++;
                                                 return ResultType::Continue;
                                             } );
        return event_loop.loop();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
