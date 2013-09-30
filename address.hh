/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ADDRESS_HH
#define ADDRESS_HH

#include <netinet/in.h>
#include <string>

class Address
{
private:
    struct sockaddr_in addr_;
public:
    enum protocol { UDP, TCP };
    Address( const struct sockaddr_in &s_addr );
    Address( const std::string hostname, const std::string service, protocol p );
    Address();

    std::string hostname( void ) const;
    uint16_t port( void ) const;
    std::string str( void ) const;

    const struct sockaddr_in & raw_sockaddr( void ) const { return addr_;}

private:
    int get_val( protocol p );
};

#endif /* ADDRESS_HH */
