/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "get_address.hh"

using namespace std;

static bool address_matches( const Address & addr, const sockaddr * candidate )
{
    return candidate and candidate->sa_family == AF_INET and addr == *candidate;
}

bool Interfaces::address_in_use( const Address & addr ) const
{
    /* iterate through list to check if input is in use */
    for ( ifaddrs *ifa = interface_addresses; ifa; ifa = ifa->ifa_next ) {
        if ( address_matches( addr, ifa->ifa_addr )
             or address_matches( addr, ifa->ifa_dstaddr ) ) {
            return true;
        }
    }

    return false;
}

pair< Address, uint16_t > Interfaces::first_unassigned_address( uint16_t last_octet ) const
{
    while ( last_octet <= 255 ) {
        Address candidate = Address::cgnat( last_octet );
        if ( !address_in_use( candidate ) ) {
            return make_pair( candidate, last_octet );
        }
        last_octet++;
    }

    throw Exception( "Interfaces", "could not find free interface address" );
}
