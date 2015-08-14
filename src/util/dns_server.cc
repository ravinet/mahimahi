/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <random>
#include <thread>
#include <chrono>
#include <string>

#include "child_process.hh"
#include "system_runner.hh"
#include "exception.hh"
#include "socket.hh"

#include "config.h"

using namespace std;

ChildProcess start_dnsmasq( const vector< string > & extra_arguments )
{
    vector< string > args = { DNSMASQ, "--keep-in-foreground", "--no-resolv",
                              "--no-hosts",
                              "-C", "/dev/null" };

    args.insert( args.end(), extra_arguments.begin(), extra_arguments.end() );

    ChildProcess dnsmasq( "dnsmasq", [&] () { return ezexec( args ); }, false, SIGTERM );

    /* busy-wait for dnsmasq to start answering DNS queries */
    UDPSocket server;
    server.connect( Address( "0", "domain" ) );

    unsigned int attempts = 0;
    while ( true ) {
        if ( ++attempts >= 20 ) {
            throw runtime_error( "dnsmasq: did not start after " + to_string( attempts ) + " attempts" );
        }

        try {
            /* write will throw an exception if the prior UDP datagram got a bad ICMP reply */
            server.write( "x" );
            this_thread::sleep_for( chrono::milliseconds( 10 ) );
            server.write( "x" );
            break;
        } catch ( const exception & e ) {
            if ( string( e.what() ).substr( 0, 6 ) != "write:" ) {
                throw;
            }
            this_thread::sleep_for( chrono::milliseconds( 10 ) );
        }
    }

    return dnsmasq;
}
