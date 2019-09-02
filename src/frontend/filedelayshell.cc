/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>

#include "file_delay_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "packetshell.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 3 ) {
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) + " delay_file_name time_resolution_ms [command...]" );
        }

        string delay_file = argv[1];
        uint64_t time_res_ms = std::stoi(argv[2]);

        vector< string > command;

        if ( argc == 3 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 2; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }
        
        PacketShell<FileDelayQueue> file_delay_shell_app( "file-delay", user_environment );

        file_delay_shell_app.start_uplink( "[delay " + delay_file + "]", command, delay_file, time_res_ms);
        file_delay_shell_app.start_downlink( delay_file, time_res_ms );
        return file_delay_shell_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
