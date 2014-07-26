/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* RAII class to make connections coming from the ingress address
   look like they're coming from the output device's address.

   We mark the connections on entry from the ingress address (with our PID),
   and then look for the mark on output. */

#include <unistd.h>

#include "nat.hh"
#include "exception.hh"
#include "config.h"

using namespace std;

NATRule::NATRule( const vector< string > & s_args )
    : arguments( s_args )
{
    vector< string > command = { IPTABLES, "-w", "-t", "nat", "-A" };
    command.insert( command.end(), arguments.begin(), arguments.end() );
    run( command );
}

NATRule::~NATRule()
{
    try {
        vector< string > command = { IPTABLES, "-w", "-t", "nat", "-D" };
        command.insert( command.end(), arguments.begin(), arguments.end() );
        run( command );
    } catch ( const exception & e ) { /* don't throw from destructor */
        print_exception( e );
    }
}

NAT::NAT( const Address & ingress_addr )
: pre_( { "PREROUTING", "-s", ingress_addr.ip(), "-j", "CONNMARK",
            "--set-mark", to_string( getpid() ) } ),
  post_( { "POSTROUTING", "-j", "MASQUERADE", "-m", "connmark",
              "--mark", to_string( getpid() ) } )
{}

DNAT::DNAT( const Address & listener, const string & interface )
    : rule_( { "PREROUTING", "-p", "TCP", "-i", interface, "-j", "DNAT",
                "--to-destination", listener.str() } )
{}
