/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef SOCKETPAIR_HH
#define SOCKETPAIR_HH

#include <utility>

#include "file_descriptor.hh"

class UnixDomainSocket : public FileDescriptor
{
public:
    using FileDescriptor::FileDescriptor;

    void send_fd( void );
    FileDescriptor recv_fd( void );

    static std::pair<UnixDomainSocket, UnixDomainSocket> make_pair( void );
};

#endif /* SOCKETPAIR_HH */
