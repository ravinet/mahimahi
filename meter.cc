/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <getopt.h>

#include "meter_queue.hh"
#include "packetshell.cc"

using namespace std;

void usage_error( const string & program_name )
{
    throw Exception( "Usage", program_name + " [--meter-uplink] [--meter-downlink] [COMMAND...]" );
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
                throw Exception( "getopt_log", "unexpected return value " + to_string( opt ) );
            }
        }

        cerr << "optind = " << optind << endl;

        vector< string > command;

        if ( optind == argc ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = optind; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        PacketShell<MeterQueue> link_shell_app( "meter" );

        link_shell_app.start_uplink( "[meter] ", user_environment, command, meter_uplink );
        link_shell_app.start_downlink( meter_downlink );
        return link_shell_app.wait_for_exit();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
