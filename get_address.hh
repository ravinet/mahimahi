/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef GET_ADDRESS_HH
#define GET_ADDRESS_HH

#include <vector>
#include <cstdint>

#include "address.hh"

class Interfaces
{
private:
    std::vector< Address > addresses_in_use;

public:
    Interfaces();

    bool address_in_use( const Address &addr ) const;

    /* find first unassigned address */
    std::pair< Address, uint16_t > first_unassigned_address( const uint16_t last_octet ) const;
};

std::pair< Address, Address > two_unassigned_addresses( void );
std::vector< std::pair< Address, Address >> get_unassigned_address_pairs( int n );

#endif /* GET_ADDRESS_HH */
