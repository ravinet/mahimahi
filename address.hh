/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ADDRESS_HH
#define ADDRESS_HH

#include <netinet/in.h>
#include <string>

#include "socket_type.hh"

class Address
{
private:
    sockaddr_in addr_;
public:
    Address( const sockaddr_in & s_addr );
    Address( const sockaddr & s_addr );
    Address( const std::string & hostname, const std::string & service, const SocketType & socket_type );
    Address( const std::string & ip, const uint16_t port );
    Address();

    static Address cgnat( const uint8_t last_octet );

    std::string ip( void ) const;
    uint16_t port( void ) const;
    std::string str( void ) const;

    const sockaddr_in & raw_sockaddr_in( void ) const { return addr_; }
    const sockaddr & raw_sockaddr( void ) const
    {
        return *reinterpret_cast<const sockaddr *>( &addr_ );
    }
    bool operator==( const Address & other ) const;
};

#endif /* ADDRESS_HH */
