/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sstream>
#include <memory>
#include <cassert>
#include <functional>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <fcntl.h>

#include "interfaces.hh"
#include "exception.hh"
#include "file_descriptor.hh"
#include "util.hh"
#include "ezio.hh"

using namespace std;

void Interfaces::add_address( const Address & addr )
{
    addresses_in_use_.emplace_back( addr );
}

Interfaces::Interfaces()
    : addresses_in_use_()
{
    /* add the ones getifaddrs can find */
    {
        /* do this in the pedantically-correct exception-safe way */
        ifaddrs *temp;
        SystemCall( "getifaddrs", getifaddrs( &temp ) );
        unique_ptr<ifaddrs, function<void(ifaddrs*)>> interface_addresses
            { temp, []( ifaddrs * x ) { freeifaddrs( x ); } };

        for ( ifaddrs *ifa = interface_addresses.get(); ifa; ifa = ifa->ifa_next ) {
            if ( ifa->ifa_addr and ifa->ifa_addr->sa_family == AF_INET ) {
                addresses_in_use_.emplace_back( *ifa->ifa_addr, sizeof( sockaddr_in ) );
            }
        }
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

    if ( all_routes.empty() ) {
        /* there are no routes, so Linux does not even make a header line! */
        return;
    }

    istringstream all_routes_stream;
    all_routes_stream.str( all_routes );

    /* read header row */
    string header_line; getline( all_routes_stream, header_line );

    if ( header_line.find( "Iface" ) != 0 ) {
        throw runtime_error( "/proc/net/route: unknown format" );
    }

    for ( string line; getline( all_routes_stream, line ); ) {
        /* parse the line */
        auto dest_start = line.find( "\t" );
        auto dest_end = line.find( "\t", dest_start + 1 );

        if ( dest_start == string::npos or dest_end == string::npos ) {
            throw runtime_error( "/proc/net/route line: unknown format" );
        }

        dest_start += 1;
        size_t len = dest_end - dest_start;

        if ( len != 8 ) {
            throw runtime_error( "/proc/net/route destination address: unknown format" );
        }

        string dest_address = line.substr( dest_start, len );

        sockaddr_in ip;
        zero( ip );
        ip.sin_family = AF_INET;
        ip.sin_addr.s_addr = myatoi( dest_address, 16 );

        addresses_in_use_.emplace_back( ip );
    }
}

bool Interfaces::address_in_use( const Address & addr ) const
{
    for ( const auto & x : addresses_in_use_ ) {
        if ( addr.ip() == x.ip() ) {
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
            assert( candidate.port() == 0 );
            return make_pair( candidate, last_octet );
        }
        last_octet++;
    }

    throw runtime_error( "Interfaces: could not find free interface address" );
}

std::pair< Address, Address > two_unassigned_addresses( const Address & avoid )
{
    Interfaces interfaces;

    interfaces.add_address( avoid );

    auto one = interfaces.first_unassigned_address( 1 );
    auto two = interfaces.first_unassigned_address( one.second + 1 );

    return make_pair( one.first, two.first );
}
