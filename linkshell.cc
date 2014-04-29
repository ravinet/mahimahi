/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "link_queue.hh"
#include "packetshell.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 5 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " uplink downlink max_buffer_size program_to_run" );
        }

        vector< string > program_to_run;
        for ( int num_args = 4; num_args < argc; num_args++ ) {
            program_to_run.emplace_back( string( argv[ num_args ] ) );
        }

        const std::string uplink_filename = argv[ 1 ];
        const std::string downlink_filename = argv[ 2 ];
        signed int max_size = atoi( argv[3] );

        PacketShell<LinkQueue> link_shell_app( "link" );

        link_shell_app.start_uplink( "[link, up=" + uplink_filename + ", down=" + downlink_filename + "] ",
                                     program_to_run, user_environment, uplink_filename, max_size );
        link_shell_app.start_downlink( downlink_filename, max_size );
        return link_shell_app.wait_for_exit();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
