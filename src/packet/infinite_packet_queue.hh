/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef INFINITE_PACKET_QUEUE_HH
#define INFINITE_PACKET_QUEUE_HH

#include <queue>
#include <cassert>

#include "queued_packet.hh"
#include "abstract_packet_queue.hh"

class InfinitePacketQueue : public AbstractPacketQueue
{
private:
    std::queue<QueuedPacket> internal_queue_ {};

public:
    void enqueue( QueuedPacket && p ) override
    {
        internal_queue_.emplace( std::move( p ) );
    }

    QueuedPacket dequeue( void ) override
    {
        assert( not internal_queue_.empty() );

        QueuedPacket ret = std::move( internal_queue_.front() );
        internal_queue_.pop();
        return ret;
    }

    bool empty( void ) const override
    {
        return internal_queue_.empty();
    }

    std::string to_string( void ) const override
    {
        return "infinite";
    }
};

#endif /* INFINITE_PACKET_QUEUE_HH */ 
