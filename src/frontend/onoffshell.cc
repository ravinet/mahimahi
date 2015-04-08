/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>
#include <iostream>

#include <getopt.h>

#include "loss_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "packetshell.cc"

using namespace std;

void usage( const string & program_name )
{
    throw runtime_error( "Usage: " + program_name + " uplink|downlink MEAN-ON-TIME MEAN-OFF-TIME [COMMAND...]" );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 4 ) {
            usage( argv[ 0 ] );
        }

        const double on_time = myatof( argv[ 2 ] );
        if ( (0 <= on_time) ) {
            /* do nothing */
        } else {
            cerr << "Error: mean on-time must be more than 0 seconds." << endl;
            usage( argv[ 0 ] );
        }

        const double off_time = myatof( argv[ 3 ] );
        if ( (0 <= off_time) ) {
            /* do nothing */
        } else {
            cerr << "Error: mean off-time must be more than 0 seconds." << endl;
            usage( argv[ 0 ] );
        }

        if ( on_time == 0 and off_time == 0 ) {
            cerr << "Error: mean on-time and off-time cannot both be 0 seconds." << endl;
            usage( argv[ 0 ] );
        }

        double uplink_on_time = numeric_limits<double>::max(), uplink_off_time = 0;
        double downlink_on_time = numeric_limits<double>::max(), downlink_off_time = 0;

        const string link = argv[ 1 ];
        if ( link == "uplink" ) {
            uplink_on_time = on_time;
            uplink_off_time = off_time;
        } else if ( link == "downlink" ) {
            downlink_on_time = on_time;
            downlink_off_time = off_time;
        } else {
            usage( argv[ 0 ] );
        }

        vector<string> command;

        if ( argc == 4 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 4; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        PacketShell<SwitchingLink> onoff_app( "onoff", user_environment );

        string shell_prefix = "[onoff ";
        if ( link == "uplink" ) {
            shell_prefix += "(up) on=";
        } else {
            shell_prefix += "(down) on=";
        }
        shell_prefix += argv[ 2 ];
        shell_prefix += "s off=";
        shell_prefix += argv[ 3 ];
        shell_prefix += "s] ";

        onoff_app.start_uplink( shell_prefix,
                                command,
                                uplink_on_time, uplink_off_time );
        onoff_app.start_downlink( downlink_on_time, downlink_off_time );
        return onoff_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
