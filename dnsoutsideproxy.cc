/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <string>
#include <iostream>

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"

using namespace std;

void service_request( const Socket & server_socket, const std::string request )
{
    /* time thread was created (socket to 127.0.0.1:53 created) */
    uint64_t time_start = timestamp();
    /* kill thread after 60 seconds */
    while ( timestamp() - time_start < 1000 ) {

        /* open socket to 127.0.0.1:53 */
        Socket outgoing_socket;
        Address outgoing_address( "127.0.0.1", "53" );

        outgoing_socket.bind( outgoing_address );
        /* send request to local dns server */
        outgoing_socket.write( request );
        /* read on outgoing_socket until response...then write to server_socket */
        string the_response = outgoing_socket.read();
        server_socket.write( the_response );
    }
    exit( EXIT_SUCCESS );
}

int main( void )
{
    /* make listener socket for dns requests */
    try {
        Socket listener_socket;
        /* 0.0.0.0 is same as INADDR_ANY (accepts connection to any local IP address) */
        Address listen_address("0.0.0.0", "0");
        listener_socket.bind( listen_address );

        uint16_t listener_port = listener_socket.local_addr().port();
        string listener_hostname = listener_socket.local_addr().hostname();        
        cout << "port: " << listener_port<< endl;
        cout << "hostname: " << listener_hostname << endl;

        while ( true ) {  /* service a request */
            string the_request = listener_socket.read(); /*  will block until we have a request */
            thread newthread( [&listener_socket] ( const string request ) -> void { service_request( listener_socket, request ); }, the_request );
            newthread.detach();
        }
    }
    catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
