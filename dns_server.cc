/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <random>
#include <thread>
#include <chrono>
#include <string>

#include "child_process.hh"
#include "system_runner.hh"
#include "util.hh"
#include "exception.hh"

#include "config.h"

using namespace std;

static const string example_ip = "127.3.1.4";

ChildProcess start_dnsmasq( const vector< string > & extra_arguments )
{
    const string random_name = "mahimahi-test-host-" + to_string( random_device()() );

    vector< string > args = { DNSMASQ, "--keep-in-foreground", "--no-resolv",
                              "--no-hosts", "-i", "lo", "--bind-interfaces",
                              "--host-record=" + random_name + "," + example_ip,
                              "-C", "/dev/null" };

    args.insert( args.end(), extra_arguments.begin(), extra_arguments.end() );

    ChildProcess dnsmasq( "dnsmasq", [&] () { return ezexec( args ); }, false, SIGTERM );

    /* busy-wait for dnsmasq to start answering DNS queries */
    unsigned int attempts = 0;
    while ( true ) {
        if ( ++attempts >= 100 ) {
            throw Exception( "dnsmasq", "did not start after " + to_string( attempts ) + " attempts" );
        }

        try {
            Address lookup( random_name, "domain" );
            if ( lookup.ip() == example_ip ) {
                break;
            }
        } catch ( const Exception & e ) {
            if ( e.attempt() != "getaddrinfo" ) {
                throw;
            }
            this_thread::sleep_for( chrono::milliseconds( 10 ) );
        }
    }

    return move( dnsmasq );
}
