/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DROPPING_PACKET_QUEUE_HH
#define DROPPING_PACKET_QUEUE_HH

#include <queue>
#include <memory>
#include <assert.h>

#include "queued_packet.hh"
#include "abstract_packet_queue.hh"

class DroppingPacketQueue : public AbstractPacketQueue
{
    protected:
    uint64_t queue_size_;
    const uint64_t packet_limit_;
    const uint64_t byte_limit_;
    std::queue<std::unique_ptr<QueuedPacket>> internal_queue_;

    public:
    DroppingPacketQueue( uint64_t s_packet_limit, uint64_t s_byte_limit)
        : queue_size_( 0 )
        , packet_limit_( s_packet_limit )
        , byte_limit_( s_byte_limit )
        , internal_queue_()
    {
        assert( (!s_byte_limit || !s_packet_limit)
                && "Don't set both byte limit and packet limit"); 
    } 

    virtual void enqueue( const QueuedPacket && ) = 0;

    std::unique_ptr<QueuedPacket> dequeue( const uint64_t )
    {
        if (internal_queue_.empty()) {
            return nullptr;
        } else {
            queue_size_ -= byte_limit_? internal_queue_.front()->contents.size() : 1;
            std::unique_ptr<QueuedPacket> ret(std::move(internal_queue_.front()));
            internal_queue_.pop();
            return ret;
        }
    }

    ~DroppingPacketQueue() {}
};

#endif /* DROPPING_PACKET_QUEUE_HH */ 
