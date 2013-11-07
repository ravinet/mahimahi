/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* RAII class to make connections coming from the ingress address
   look like they're coming from the output device's address.

   We mark the connections on entry from the ingress address (with our PID),
   and then look for the mark on output. */

#include <unistd.h>

#include "nat.hh"
#include "exception.hh"

NATRule::NATRule( const std::vector< std::string > & s_args )
    : arguments( s_args )
{
    std::vector< std::string > command = { IPTABLES, "-t", "nat", "-A" };
    command.insert( command.end(), arguments.begin(), arguments.end() );
    run( command );
}

NATRule::~NATRule()
{
    try {
        std::vector< std::string > command = { IPTABLES, "-t", "nat", "-D" };
        command.insert( command.end(), arguments.begin(), arguments.end() );
        run( command );
    } catch ( const Exception & e ) { /* don't throw from destructor */
        e.perror();
    }
}

NAT::NAT( const Address & ingress_addr )
: pre_( { "PREROUTING", "-s", ingress_addr.ip(), "-j", "CONNMARK",
            "--set-mark", std::to_string( getpid() ) } ),
  post_( { "POSTROUTING", "-j", "MASQUERADE", "-m", "connmark",
              "--mark", std::to_string( getpid() ) } )
{}

DNAT::DNAT( const Address & listener, const std::string & interface )
    : rule_( { "PREROUTING", "-p", "TCP", "-i", interface, "-j", "DNAT",
                "--to-destination", listener.str() } )
{}
