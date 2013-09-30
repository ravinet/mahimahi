/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef SOCKET_HH
#define SOCKET_HH

#include <stdint.h>
#include <string>
#include <utility>
#include <sys/types.h>
#include <sys/socket.h>

#include "address.hh"
#include "file_descriptor.hh"

class Socket
{
private:
    FileDescriptor fd_;

    Address local_addr_, peer_addr_;

    const int listen_backlog_ = 16;

public:
    Socket() = delete; /* default constructor */
    enum protocol { UDP, TCP };
    //Socket();
    Socket( protocol p );

    Socket( FileDescriptor && s_fd, const Address & s_local_addr, const Address & s_peer_addr );

    void bind( const Address & addr );
    void connect( const Address & addr );
    void listen( void );
    Socket accept( void ) const;

    const Address & local_addr( void ) const { return local_addr_; }
    const Address & peer_addr( void ) const { return peer_addr_; }

    std::string read( void ) const;
    void write( const std::string & str ) const;

    int raw_fd( void ) const { return fd_.num(); }

    std::pair< Address, std::string > recvfrom( void ) const;
    void sendto( const Address & destination, const std::string & payload ) const;

private:
    int get_val( protocol p );
};

#endif
