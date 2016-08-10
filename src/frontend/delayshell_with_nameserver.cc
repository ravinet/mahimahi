/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>

#include "delay_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "packetshell.cc"
#include "vpn.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 3 ) {
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) + " delay-milliseconds nameserver [command...]" );
        }

        const uint64_t delay_ms = myatoi( argv[ 1 ] );

        string nameserver_ip = argv[2];

        Address nameserver_address(nameserver_ip, 53);

        vector< string > command; // This should be OpenVPN but will be handled in packetshell

        PacketShell<DelayQueue> delay_shell_app( "delay", user_environment );

        delay_shell_app.start_uplink_and_forward_packets_with_nameserver
          ( "[delay " + to_string( delay_ms ) + " ms] ",
            1194 /* openvpn port */, nameserver_address,
            command, delay_ms );
        delay_shell_app.start_downlink( delay_ms );
        return delay_shell_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
