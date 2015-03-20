/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <getopt.h>

#include "link_queue.hh"
#include "packetshell.cc"

using namespace std;

void usage_error( const string & program_name )
{
    throw runtime_error( "Usage: " + program_name + " [--uplink-log=FILENAME] [--downlink-log=FILENAME] [--meter-uplink] [--meter-uplink-delay] [--meter-downlink] [--meter-downlink-delay] [--once] UPLINK DOWNLINK [COMMAND...]" );
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
            { "uplink-log",     required_argument, nullptr, 'u' },
            { "downlink-log",   required_argument, nullptr, 'd' },
            { "once",           no_argument,       nullptr, 'o' },
            { "meter-uplink",   no_argument,       nullptr, 'm' },
            { "meter-downlink", no_argument,       nullptr, 'n' },
            { "meter-uplink-delay",   no_argument,       nullptr, 'x' },
            { "meter-downlink-delay", no_argument,       nullptr, 'y' },
            { 0,                0,                 nullptr, 0 }
        };

        string uplink_logfile, downlink_logfile;
        bool repeat = true;
        bool meter_uplink = false, meter_downlink = false;
        bool meter_uplink_delay = false, meter_downlink_delay = false;

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
            case 'o':
                repeat = false;
                break;
            case 'm':
                meter_uplink = true;
                break;
            case 'n':
                meter_downlink = true;
                break;
            case 'x':
                meter_uplink_delay = true;
                break;
            case 'y':
                meter_downlink_delay = true;
                break;
            case '?':
                usage_error( argv[ 0 ] );
                break;
            default:
                throw runtime_error( "getopt_log: unexpected return value " + to_string( opt ) );
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

        PacketShell<LinkQueue> link_shell_app( "link", user_environment );

        const string uplink_name = "Uplink", downlink_name = "Downlink";

        link_shell_app.start_uplink( "[link] ", command,
                                     uplink_name, uplink_filename, uplink_logfile, repeat, meter_uplink, meter_uplink_delay );
        link_shell_app.start_downlink( downlink_name, downlink_filename, downlink_logfile, repeat, meter_downlink, meter_downlink_delay );
        return link_shell_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
