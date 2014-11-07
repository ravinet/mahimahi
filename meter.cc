/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <getopt.h>

#include "meter_queue.hh"
#include "packetshell.cc"

using namespace std;

void usage_error( const string & program_name )
{
    throw runtime_error( "Usage: " + program_name + " [--meter-uplink] [--meter-downlink] [COMMAND...]" );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        const option command_line_options[] = {
            { "meter-uplink",   no_argument, nullptr, 'u' },
            { "meter-downlink", no_argument, nullptr, 'd' },
            { 0,                0,           nullptr, 0 }
        };

        bool meter_uplink = false, meter_downlink = false;

        while ( true ) {
            const int opt = getopt_long( argc, argv, "ud", command_line_options, nullptr );
            if ( opt == -1 ) { /* end of options */
                break;
            }

            switch ( opt ) {
            case 'u':
                meter_uplink = true;
                break;
            case 'd':
                meter_downlink = true;
                break;
            case '?':
                usage_error( argv[ 0 ] );
                break;
            default:
                throw runtime_error( "getopt_log: unexpected return value " + to_string( opt ) );
            }
        }

        vector< string > command;

        if ( optind == argc ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = optind; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        PacketShell<MeterQueue> link_shell_app( "meter", user_environment );

        const string uplink_name = "Uplink", downlink_name = "Downlink";

        link_shell_app.start_uplink( "[meter] ", command,
                                     uplink_name, meter_uplink );
        link_shell_app.start_downlink( downlink_name, meter_downlink );
        return link_shell_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
