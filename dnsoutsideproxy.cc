/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <string>
#include <iostream>

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"

using namespace std;


//service_request will open a new socket to 127.0.0.1:53, send the request there, and then read() on that socket until it gets a response, and when it does, it will write() that back to server_socket.
/*void service_request( const Socket & server_socket, const std::string request )
{
    // open socket to 127.0.0.1:53
    uint64_t time_start = timestamp();
    Socket outgoing_socket;
    Address outgoing_address( "127.0.0.1", 53 );
    int outgoing_port = outgoing_socket.bind( outgoing_address );
    cout << outgoing_port << endl;
    cout << time_start << endl;
    // send request to 
    outgoing_socket.write( request );    

    // read on outgoing_socket until response...then write to server_socket
    string the_response = outgoing_socket.read();
    server_socket.write( the_response ); 


}*/

int main( void )
{
    /* make listener socket for dns requests */
    try {
    Socket listener_socket;
    //cout << listener_socket.raw_fd();
    Address listen_address( 0 );
    //int port = listener_socket.bind( listen_address );
    //listener_socket.bind( listen_address );
    //cout << port << endl;
    }
    catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    /*while ( true ) {  // service a request
        string the_request = listener_socket.read(); //  will block until we have a request
        thread newthread( [&listener_socket] ( const string request ) -> void { service_request( listener_socket, request ); }, the_request );
        newthread.detach();
    }*/
    //return EXIT_SUCCESS;

}
