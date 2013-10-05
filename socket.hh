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
#include "socket_type.hh"

class Socket
{
private:
    FileDescriptor fd_;

    Address local_addr_, peer_addr_;

public:
    Socket( const SocketType & socket_type );

    Socket( FileDescriptor && s_fd, const Address & s_local_addr, const Address & s_peer_addr );

    void bind( const Address & addr );
    void connect( const Address & addr );
    void listen( void );
    Socket accept( void );

    const Address & local_addr( void ) const { return local_addr_; }
    const Address & peer_addr( void ) const { return peer_addr_; }

    std::string read( void );
    void write( const std::string & str );

    int raw_fd( void ) { return fd_.num(); }

    std::pair< Address, std::string > recvfrom( void );
    void sendto( const Address & destination, const std::string & payload );
};

#endif
