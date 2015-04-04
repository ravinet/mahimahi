/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef INFINITE_PACKET_QUEUE_HH
#define INFINITE_PACKET_QUEUE_HH

#include <queue>

#include "queued_packet.hh"
#include "abstract_packet_queue.hh"

class InfinitePacketQueue : public AbstractPacketQueue
{
    std::queue<std::unique_ptr<QueuedPacket>> internal_queue_;

    public:
    InfinitePacketQueue() : internal_queue_() {} 

    void enqueue( const QueuedPacket && p )
    {
        internal_queue_.emplace( new QueuedPacket( std::move(p) ) );
    }

    std::unique_ptr<QueuedPacket> dequeue( const uint64_t )
    {
        if (internal_queue_.empty()) {
            return nullptr;
        } else {
            std::unique_ptr<QueuedPacket> ret(std::move(internal_queue_.front()));
            internal_queue_.pop();
            return ret;
        }
    }

    ~InfinitePacketQueue() {}
};

#endif /* INFINITE_PACKET_QUEUE_HH */ 
