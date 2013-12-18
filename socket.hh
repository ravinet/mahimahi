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
#include "read_write_interface.hh"
#include "socket_type.hh"

class Socket : public ReadWriteInterface
{
private:
    FileDescriptor fd_;

    Address local_addr_, peer_addr_;

    void getsockopt( const int level, const int optname,
                     void *optval, socklen_t *optlen ) const;

public:
    Socket( const SocketType & socket_type );

    Socket( FileDescriptor && s_fd, const Address & s_local_addr, const Address & s_peer_addr );

    /* forbid copying */
    Socket( const Socket & other ) = delete;

    /* allow moving */
    Socket( Socket && other );

    void bind( const Address & addr );
    void connect( const Address & addr );
    void listen( void );
    Socket accept( void );

    const Address & local_addr( void ) const { return local_addr_; }
    const Address & peer_addr( void ) const { return peer_addr_; }

    std::string read( void ) override;
    std::string read ( const size_t limit );
    void write( const std::string & str ) override;
    std::string::const_iterator write_some( const std::string::const_iterator & begin,
                                            const std::string::const_iterator & end );

    FileDescriptor & fd( void ) override { return fd_; }

    std::pair< Address, std::string > recvfrom( void );
    void sendto( const Address & destination, const std::string & payload );

    Address original_dest( void ) const;
};

#endif
