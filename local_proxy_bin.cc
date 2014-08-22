/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <utility>

#include "poller.hh"
#include "local_proxy.hh"

using namespace std;
using namespace PollerShortNames;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 5 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " local_proxy_ip local_proxy_port remote_proxy_ip remote_proxy_port" );
        }

        Address listener_addr( string( argv[ 1 ] ), argv[ 2 ] );
        Address remote_proxy_addr( string( argv[ 3 ] ), argv[ 4 ] );

        LocalProxy local_proxy( listener_addr, remote_proxy_addr );

        EventLoop event_loop;

        local_proxy.register_handlers( event_loop );

        event_loop.loop();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
