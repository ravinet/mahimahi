/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FERRY_HH
#define FERRY_HH

/* convey delayed packets between a file descriptor (probably a tap device)
   and a sibling fd.

   watch for events on a child_process and respond appropriately. */

#include "event_loop.hh"

class Ferry : public EventLoop
{
public:
    template <class FerryQueueType>
    int loop( FerryQueueType & ferry_queue,
              FileDescriptor & tun,
              FileDescriptor & sibling_fd );
};

#endif /* FERRY_HH */
