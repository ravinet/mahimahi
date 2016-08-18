/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* RAII class to make connections coming from the ingress address
   look like they're coming from the output device's address.

   We mark the connections on entry from the ingress address (with our PID),
   and then look for the mark on output. */

#include <unistd.h>
#include <iostream>

#include "nat.hh"
#include "exception.hh"
#include "config.h"

using namespace std;

NATRule::NATRule( const vector< string > & s_args )
    : arguments( s_args )
{
    if (!arguments.empty()) {
      vector< string > command = { IPTABLES, "-w", "-t", "nat", "-I" };
      command.insert( command.end(), arguments.begin(), arguments.end() );
      cout << "command: ";
      for (auto i = command.begin(); i != command.end(); ++i)
           cout << *i << ' ';
      cout << endl;
      run( command );
    }
}

NATRule::NATRule( const vector< string > & s_args, bool as_sudo )
    : arguments( s_args )
{
  if (!arguments.empty()) {
    vector< string > command;
    if (as_sudo) {
      command = { "sudo", IPTABLES, "-w", "-t", "nat", "-I" };
    } else {
      command = { IPTABLES, "-w", "-t", "nat", "-I" };
    }
    command.insert( command.end(), arguments.begin(), arguments.end() );
    cout << "command: ";
    for (auto i = command.begin(); i != command.end(); ++i)
         cout << *i << ' ';
    cout << endl;
    run( command );
  }
}

NATRule::~NATRule()
{
    try {
      if (!arguments.empty()) {
        cout << "[nat] Removing NAT rule" << endl;
        vector< string > command = { IPTABLES, "-w", "-t", "nat", "-D" };
        for (auto it = arguments.begin(); it != arguments.end(); ++it) {
          auto argument = *it;
          if (argument != "1") {
            command.push_back(*it);
          }
        }
        // command.insert( command.end(), arguments.begin(), arguments.end() );
        run( command );
      }
    } catch ( const exception & e ) { /* don't throw from destructor */
        print_exception( e );
    }
}

NAT::NAT( const Address & ingress_addr )
: pre_( { "PREROUTING", "1", "-s", ingress_addr.ip(), "-j", "CONNMARK",
            "--set-mark", to_string( getpid() ) } ),
  post_( { "POSTROUTING", "1", "-j", "MASQUERADE", "-m", "connmark",
              "--mark", to_string( getpid() ) } )
{}

NAT::NAT()
: pre_( { } ),
  post_( { } )
{}

DNAT::DNAT()
  : rule_( { } )
{}

DNAT::DNAT( const Address & listener, const string & interface )
    : rule_( { "PREROUTING", "1", "-p", "TCP", "-i", interface, "-j", "DNAT",
                "--to-destination", listener.str() } )
{}

DNAT::DNAT( const Address & listener, const uint16_t port )
    : rule_( { "PREROUTING", "1", "-p", "TCP", "--dport", to_string(port), "-j", "DNAT",
                "--to-destination", listener.str() } )
{}

DNAT::DNAT( const Address & listener, const string & protocol, const uint16_t port )
    : rule_( { "PREROUTING", "1", "-p", protocol, "--dport", to_string(port), "-j", "DNAT",
                "--to-destination", listener.str() } )
{}

DNATWithPostrouting::DNATWithPostrouting( const Address & listener, const string & protocol, const uint16_t port )
    : rule_( { "PREROUTING", "1", "-p", protocol, "--dport", to_string(port), "-j", "DNAT",
                "--to-destination", listener.str() } ),
      post_( { "POSTROUTING", "1", "-j", "MASQUERADE" } )
{}

DNATWithPostrouting::DNATWithPostrouting()
  : rule_( { } ),
    post_( { } )
{}
