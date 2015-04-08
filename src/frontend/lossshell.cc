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
    throw runtime_error( "Usage: " + program_name + " uplink|downlink RATE [COMMAND...]" );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 3 ) {
            usage( argv[ 0 ] );
        }

        const double loss_rate = myatof( argv[ 2 ] );
        if ( (0 <= loss_rate) and (loss_rate <= 1) ) {
            /* do nothing */
        } else {
            cerr << "Error: loss rate must be between 0 and 1." << endl;
            usage( argv[ 0 ] );
        }

        double uplink_loss = 0, downlink_loss = 0;

        const string link = argv[ 1 ];
        if ( link == "uplink" ) {
            uplink_loss = loss_rate;
        } else if ( link == "downlink" ) {
            downlink_loss = loss_rate;
        } else {
            usage( argv[ 0 ] );
        }

        vector<string> command;

        if ( argc == 3 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 3; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        PacketShell<IIDLoss> loss_app( "loss", user_environment );

        string shell_prefix = "[loss ";
        if ( link == "uplink" ) {
            shell_prefix += "up=";
        } else {
            shell_prefix += "down=";
        }
        shell_prefix += argv[ 2 ];
        shell_prefix += "] ";

        loss_app.start_uplink( shell_prefix,
                               command,
                               uplink_loss );
        loss_app.start_downlink( downlink_loss );
        return loss_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
