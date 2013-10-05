/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef GET_ADDRESS_HH
#define GET_ADDRESS_HH

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include "exception.hh"
#include "address.hh"

class Interfaces
{
private:
    ifaddrs *interface_addresses;

public:
    Interfaces()
      : interface_addresses()
    {
        if ( getifaddrs( &interface_addresses ) < 0 ) {
            throw Exception( "getifaddrs" );
        }
    }

    ~Interfaces()
    {
        freeifaddrs( interface_addresses );
    }

    bool address_in_use( const Address &addr ) const;

    const Interfaces & operator=( const Interfaces & other ) = delete;
    Interfaces( const Interfaces & other ) = delete;

    /* find first unassigned address */
    std::pair< Address, uint16_t > first_unassigned_address( const uint16_t last_octet ) const;
};

#endif /* GET_ADDRESS_HH */
