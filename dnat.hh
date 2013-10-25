/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DNAT_HH
#define DNAT_HH

/* DNAT for HTTP Proxy */
#include <string>

#include "config.h"
#include "system_runner.hh"
#include "address.hh"

class DNAT
{
private:
    Address listener;
    std::string interface_to_forward_to;

public:
    DNAT ( Address egress, std::string interface )
        : listener( egress ),
          interface_to_forward_to( interface ) 
    {
        run( { IPTABLES, "-t", "nat", "-A", "PREROUTING", "-p", "TCP", "-i", interface_to_forward_to, "-j", "DNAT", "--to-destination", listener.ip() + ":" + std::to_string( listener.port() ) } );
    }

    ~DNAT()
    {
        run( { IPTABLES, "-t", "nat", "-D", "PREROUTING", "-p", "TCP", "-i", interface_to_forward_to, "-j", "DNAT", "--to-destination", listener.ip() + ":" + std::to_string( listener.port() ) } );
    }
};

#endif /* DNAT_HH */
