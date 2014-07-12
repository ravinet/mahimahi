/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "socketpair.hh"
#include "util.hh"

using namespace std;

pair<UnixDomainSocket, UnixDomainSocket> UnixDomainSocket::make_pair( void )
{
    int pipe[ 2 ];
    SystemCall( "socketpair", socketpair( AF_UNIX, SOCK_DGRAM, 0, pipe ) );
    return ::make_pair( UnixDomainSocket( pipe[ 0 ] ), UnixDomainSocket( pipe[ 1 ] ) );
}

static const char fd_signal = '#';

void UnixDomainSocket::send_fd( FileDescriptor & fd )
{
    char mutable_char_to_send = fd_signal;
    iovec msg_iovec = { &mutable_char_to_send, sizeof( mutable_char_to_send ) };

    char control_buffer[ CMSG_SPACE( sizeof( fd.num() ) ) ];

    msghdr message_header;
    zero( message_header );

    message_header.msg_iov = &msg_iovec;
    message_header.msg_iovlen = 1;
    message_header.msg_control = control_buffer;
    message_header.msg_controllen = sizeof( control_buffer );

    cmsghdr * const control_message = CMSG_FIRSTHDR( &message_header );
    control_message->cmsg_level = SOL_SOCKET;
    control_message->cmsg_type = SCM_RIGHTS;
    control_message->cmsg_len = CMSG_LEN( sizeof( fd.num() ) );
    *reinterpret_cast<int *>( CMSG_DATA( control_message ) ) = fd.num();
    message_header.msg_controllen = control_message->cmsg_len;

    if ( 1 != SystemCall( "sendmsg", sendmsg( num(), &message_header, 0 ) ) ) {
        throw Exception( "send_fd", "sendmsg did not send expected number of bytes" );
    }
}

FileDescriptor UnixDomainSocket::recv_fd( void )
{
    char mutable_char_to_receive;
    iovec msg_iovec = { &mutable_char_to_receive, sizeof( mutable_char_to_receive ) };

    char control_buffer[ CMSG_SPACE( sizeof( int ) ) ];

    msghdr message_header;
    zero( message_header );

    message_header.msg_iov = &msg_iovec;
    message_header.msg_iovlen = 1;
    message_header.msg_control = control_buffer;
    message_header.msg_controllen = sizeof( control_buffer );

    if ( 1 != SystemCall( "recvmsg", recvmsg( num(), &message_header, 0 ) ) ) {
        throw Exception( "recv_fd", "recvmsg did not receive expected number of bytes" );
    }

    if ( mutable_char_to_receive != fd_signal ) {
        throw Exception( "recv_fd", "did not get expected fd signal byte" );
    }

    if ( message_header.msg_flags & MSG_CTRUNC ) {
        throw Exception( "recvmsg", "control data was truncated" );
    }

    const cmsghdr * const control_message = CMSG_FIRSTHDR( &message_header );
    if ( (not control_message)
         or (control_message->cmsg_level != SOL_SOCKET)
         or (control_message->cmsg_type != SCM_RIGHTS) ) {
        throw Exception( "recvmsg", "unexpected control message" );
    }

    if ( control_message->cmsg_len != CMSG_LEN( sizeof( int ) ) ) {
        throw Exception( "recvmsg", "unexpected control message length" );
    }

    return *reinterpret_cast<int *>( CMSG_DATA( control_message ) );
}
