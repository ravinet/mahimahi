/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef SOCKET_HH
#define SOCKET_HH

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

#include "address.hh"
#include "file_descriptor.hh"
#include "socket_type.hh"

class Socket : public FileDescriptor
{
private:
    Address local_addr_, peer_addr_;

    void getsockopt( const int level, const int optname,
                     void *optval, socklen_t *optlen ) const;

public:
    Socket( const SocketType & socket_type );

    Socket( FileDescriptor && s_fd, const Address & s_local_addr, const Address & s_peer_addr );

    /* forbid copying or assignment */
    Socket( const Socket & other ) = delete;
    const Socket & operator=( const Socket & other ) = delete;

    /* allow moving */
    Socket( Socket && other );

    void bind( const Address & addr );
    void connect( const Address & addr );
    void listen( void );
    Socket accept( void );

    const Address & local_addr( void ) const { return local_addr_; }
    const Address & peer_addr( void ) const { return peer_addr_; }

    std::pair< Address, std::string > recvfrom( void );
    void sendto( const Address & destination, const std::string & payload );

    Address original_dest( void ) const;
};

#endif
