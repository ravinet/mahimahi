/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include "get_address.hh"
#include "exception.hh"
#include "file_descriptor.hh"
#include "util.hh"

using namespace std;

Interfaces::Interfaces()
    : addresses_in_use()
{
    /* add the ones getifaddrs can find */
    {
        ifaddrs *interface_addresses;
        SystemCall( "getifaddrs", getifaddrs( &interface_addresses ) );

        for ( ifaddrs *ifa = interface_addresses; ifa; ifa = ifa->ifa_next ) {
            if ( ifa->ifa_addr and ifa->ifa_addr->sa_family == AF_INET ) {
                addresses_in_use.emplace_back( *ifa->ifa_addr );
            }
        }
        freeifaddrs( interface_addresses );
    }

    /* Now also add route destinations. */

    /* Unfortunately getifaddrs does not see the destination address of a
       veth interface, so read this from /proc. We could also use rtnetlink. */

    FileDescriptor routes( SystemCall( "open /proc/net/route",
                                       open( "/proc/net/route", O_RDONLY ) ) );

    /* read routes */
    string all_routes;
    while ( true ) {
        string new_chunk = routes.read();
        if ( new_chunk.empty() ) { break; }
        all_routes += new_chunk;
    }

    istringstream all_routes_stream;
    all_routes_stream.str( all_routes );

    /* read header row */
    string header_line; getline( all_routes_stream, header_line );

    if ( header_line.find( "Iface" ) != 0 ) {
        throw Exception( "/proc/net/route", "unknown format" );
    }

    for ( string line; getline( all_routes_stream, line ); ) {
        /* parse the line */
        auto dest_start = line.find( "\t" );
        auto dest_end = line.find( "\t", dest_start + 1 );

        if ( dest_start == string::npos or dest_end == string::npos ) {
            throw Exception( "/proc/net/route line", "unknown format" );
        }

        dest_start += 1;
        size_t len = dest_end - dest_start;

        if ( len != 8 ) {
            throw Exception( "/proc/net/route destination address", "unknown format" );
        }

        string dest_address = line.substr( dest_start, len );

        sockaddr_in ip;
        zero( ip );
        ip.sin_family = AF_INET;
        ip.sin_addr.s_addr = myatoi( dest_address, 16 );

        addresses_in_use.emplace_back( ip );
    }
}

bool Interfaces::address_in_use( const Address & addr ) const
{
    for ( const auto & x : addresses_in_use ) {
        if ( addr == x ) {
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

std::pair< Address, Address > two_unassigned_addresses( void )
{
    static Interfaces interface_list_on_program_startup;

    auto one = interface_list_on_program_startup.first_unassigned_address( 1 );
    auto two = interface_list_on_program_startup.first_unassigned_address( one.second + 1 );

    return make_pair( one.first, two.first );
}

std::vector< std::pair< Address, Address >> get_unassigned_address_pairs( int n )
{
    static Interfaces interface_list_on_program_startup;

    std::vector< std::pair< Address, Address >> ret;
    int last_octet = 1;
    for (int i = 1; i <= n; i++) {
      auto one = interface_list_on_program_startup.first_unassigned_address( last_octet );
      auto two = interface_list_on_program_startup.first_unassigned_address( one.second + 1 );
      last_octet = two.second + 1;
      ret.emplace_back( make_pair( one.first, two.first ) );
    }
    return ret;
}
