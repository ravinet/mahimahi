/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ABSTRACT_PACKET_QUEUE
#define ABSTRACT_PACKET_QUEUE

#include <queue>
#include <memory>

#include "queued_packet.hh"

class AbstractPacketQueue
{
    public:
    virtual void enqueue( const QueuedPacket && p ) = 0;

    virtual std::unique_ptr<QueuedPacket> dequeue( const uint64_t ) = 0;

    virtual ~AbstractPacketQueue() = default;
};

#endif /* ABSTRACT_PACKET_QUEUE */ 
