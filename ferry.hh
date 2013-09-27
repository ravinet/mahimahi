/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FERRY_HH
#define FERRY_HH

/* convey delayed packets between a file descriptor (probably a tap device)
   and a sibling fd.

   watch for events on a child_process and respond appropriately.

   the ferry() routine loops until exit. */

#include "file_descriptor.hh"
#include "child_process.hh"
#include "socket.hh"
#include "address.hh"

int ferry( const FileDescriptor & tun,
           const FileDescriptor & sibling_fd,
           const Socket & listen_socket,
           const Address connect_addr,
           ChildProcess & child_process,
           const uint64_t delay_ms );

#endif /* FERRY_HH */
