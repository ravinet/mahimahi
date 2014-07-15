/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <getopt.h>

#include "link_queue.hh"
#include "packetshell.cc"

using namespace std;

void usage_error( const string & program_name )
{
    throw Exception( "Usage", program_name + " [--uplink-log=FILENAME] [--downlink-log=FILENAME] UPLINK DOWNLINK [COMMAND...]" );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 3 ) {
            usage_error( argv[ 0 ] );
        }

        const option command_line_options[] = {
            { "uplink-log", required_argument, nullptr, 'u' },
            { "downlink-log", required_argument, nullptr, 'd' },
            { 0,         0,                 nullptr, 0 }
        };

        string uplink_logfile, downlink_logfile;

        while ( true ) {
            const int opt = getopt_long( argc, argv, "u:d:", command_line_options, nullptr );
            if ( opt == -1 ) { /* end of options */
                break;
            }

            switch ( opt ) {
            case 'u':
                uplink_logfile = optarg;
                break;
            case 'd':
                downlink_logfile = optarg;
                break;
            case '?':
                usage_error( argv[ 0 ] );
                break;
            default:
                throw Exception( "getopt_log", "unexpected return value " + to_string( opt ) );
            }
        }

        if ( optind + 1 >= argc ) {
            usage_error( argv[ 0 ] );
        }

        const std::string uplink_filename = argv[ optind ];
        const std::string downlink_filename = argv[ optind + 1 ];

        vector< string > command;

        if ( optind + 2 == argc ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = optind + 2; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        PacketShell<LinkQueue> link_shell_app( "link" );

        link_shell_app.start_uplink( "[link] ", user_environment, command,
                                     uplink_filename, uplink_logfile );
        link_shell_app.start_downlink( downlink_filename, downlink_logfile );
        return link_shell_app.wait_for_exit();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
