/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "packetshell.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 7 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " cell_delay cell_uplink cell_downlink wifi_delay wifi_uplink wifi_downlink " );
        }

        const uint64_t cell_delay = myatoi( argv[ 1 ] );
        const std::string cell_uplink = argv[ 2 ];
        const std::string cell_downlink = argv[ 3 ];
        const uint64_t wifi_delay = myatoi( argv[ 4 ] );
        const std::string wifi_uplink = argv[ 5 ];
        const std::string wifi_downlink = argv[ 6 ];

        PacketShell rate_delay_shell_app( "cd" );

        rate_delay_shell_app.start_uplink( to_string(cell_delay) + cell_uplink + cell_downlink + to_string(wifi_delay) + wifi_uplink + wifi_downlink,
                                           user_environment,
                                           cell_delay,
                                           wifi_delay,
                                           cell_uplink,
                                           wifi_uplink);

        rate_delay_shell_app.start_downlink( cell_delay, wifi_delay, cell_downlink, wifi_downlink );
        return rate_delay_shell_app.wait_for_exit();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
