/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FERRY_HH
#define FERRY_HH

/* convey delayed packets between a file descriptor (probably a tap device)
   and a sibling fd.

   watch for events on a child_process and respond appropriately.

   the ferry() routine loops until exit. */

#include <memory>

#include "file_descriptor.hh"
#include "child_process.hh"
#include "dns_proxy.hh"
#include "http_proxy.hh"

template <class FerryType>
int packet_ferry( FerryType & ferry,
                  FileDescriptor & tun,
                  FileDescriptor & sibling_fd,
                  std::unique_ptr<DNSProxy> && dns_proxy,
                  std::vector<ChildProcess> && child_processes );

template <class FerryType>
int packet_ferry( FerryType & ferry,
                  FileDescriptor & tun,
                  FileDescriptor & sibling_fd,
                  std::unique_ptr<DNSProxy> && dns_proxy,
                  ChildProcess && child_process );

#endif /* FERRY_HH */
