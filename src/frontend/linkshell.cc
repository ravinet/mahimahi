/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <getopt.h>

#include "infinite_packet_queue.hh"
#include "drop_tail_packet_queue.hh"
#include "drop_head_packet_queue.hh"
#include "link_queue.hh"
#include "packetshell.cc"

using namespace std;

void usage_error( const string & program_name )
{
    throw runtime_error( "Usage: " + program_name + " [--uplink-log=FILENAME] [--downlink-log=FILENAME] [--meter-uplink] [--meter-uplink-delay] [--meter-downlink] [--meter-downlink-delay] [--uplink-queue=droptail_bytelimit|droptail_packetlimit|drophead_bytelimit|drophead_packetlimit] [--downlink-queue=droptail_bytelimit|droptail_packetlimit|drophead_bytelimit|drophead_packetlimit] [--uplink-queue-args=NUMBER] [--downlink-queue-args=NUMBER] [--once] UPLINK DOWNLINK [COMMAND...]" );
}

unique_ptr<AbstractPacketQueue> get_packet_queue( const string & queue_arg, const string & queue_params, const string & program_name )
{
    if ( queue_arg.empty() && queue_params.empty() ) {
        cout<< "defaulting to infinite queue" << endl;
        return unique_ptr<AbstractPacketQueue>( new InfinitePacketQueue() );
    } else if ( queue_arg.empty() || queue_params.empty() ) {
        usage_error( program_name );
    }

    uint64_t params = 0;
    try {
        params = stoul( queue_params );
    } catch (...) {
        usage_error( program_name );
    }

    if (!params)
    {
        usage_error( program_name );
    }

    if (queue_arg.compare("droptail_bytelimit") == 0) {
        return unique_ptr<AbstractPacketQueue>(new DropTailPacketQueue( 0, params ) );
    } else if (queue_arg.compare("droptail_packetlimit") == 0) {
        return unique_ptr<AbstractPacketQueue>(new DropTailPacketQueue( params, 0 ) );
    } else if (queue_arg.compare("drophead_bytelimit") == 0) {
        return unique_ptr<AbstractPacketQueue>(new DropHeadPacketQueue( 0, params ) );
    } else if (queue_arg.compare("drophead_packetlimit") == 0) {
        return unique_ptr<AbstractPacketQueue>(new DropHeadPacketQueue( params, 0 ) ); 
    } 

    usage_error( program_name );
    return nullptr;
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
            { "uplink-log",           required_argument, nullptr, 'u' },
            { "downlink-log",         required_argument, nullptr, 'd' },
            { "once",                       no_argument, nullptr, 'o' },
            { "meter-uplink",               no_argument, nullptr, 'm' },
            { "meter-downlink",             no_argument, nullptr, 'n' },
            { "meter-uplink-delay",         no_argument, nullptr, 'x' },
            { "meter-downlink-delay",       no_argument, nullptr, 'y' },
            { "uplink-queue",         required_argument, nullptr, 'q' },
            { "downlink-queue",       required_argument, nullptr, 'w' },
            { "uplink-queue-args",    required_argument, nullptr, 'a' },
            { "downlink-queue-args",  required_argument, nullptr, 'b' },
            { 0,                                      0, nullptr, 0 }
        };

        string uplink_logfile, downlink_logfile;
        bool repeat = true;
        bool meter_uplink = false, meter_downlink = false;
        bool meter_uplink_delay = false, meter_downlink_delay = false;
        string uplink_queue_type, downlink_queue_type,
               uplink_queue_args, downlink_queue_args;

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
            case 'q':
                uplink_queue_type = optarg; 
                break;
            case 'w':
                downlink_queue_type = optarg;
                break;
            case 'a':
                uplink_queue_args = optarg;
                break;
            case 'b':
                downlink_queue_args = optarg;
                break;
            case '?':
                usage_error( argv[ 0 ] );
                break;
            default:
                throw runtime_error( "getopt_long: unexpected return value " + to_string( opt ) );
            }
        }

        if ( optind + 1 >= argc ) {
            usage_error( argv[ 0 ] );
        }

        const string uplink_filename = argv[ optind ];
        const string downlink_filename = argv[ optind + 1 ];

        vector< string > command;

        if ( optind + 2 == argc ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = optind + 2; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        PacketShell<LinkQueue> link_shell_app( "link", user_environment );

        link_shell_app.start_uplink( "[link] ", command,
                                     "Uplink", uplink_filename, uplink_logfile, repeat, meter_uplink, meter_uplink_delay,
                                     get_packet_queue( uplink_queue_type, uplink_queue_args, argv[ 0 ] ) );

        link_shell_app.start_downlink( "Downlink", downlink_filename, downlink_logfile, repeat, meter_downlink, meter_downlink_delay,
                                       get_packet_queue( downlink_queue_type, downlink_queue_args, argv[ 0 ] ) );

        return link_shell_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
