/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <sys/un.h>

#include "socketpair.hh"

using namespace std;

std::pair<UnixDomainSocket, UnixDomainSocket> UnixDomainSocket::make_pair( void )
{
    int pipe[ 2 ];
    SystemCall( "socketpair", socketpair( AF_UNIX, SOCK_DGRAM, 0, pipe ) );
    return ::make_pair( UnixDomainSocket( pipe[ 0 ] ), UnixDomainSocket( pipe[ 1 ] ) );
}
