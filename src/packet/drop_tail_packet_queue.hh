/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DROP_TAIL_PACKET_QUEUE_HH
#define DROP_TAIL_PACKET_QUEUE_HH

#include "dropping_packet_queue.hh"

class DropTailPacketQueue : public DroppingPacketQueue
{
    public:
    DropTailPacketQueue( uint64_t s_packet_limit, uint64_t s_byte_limit)
        : DroppingPacketQueue(s_packet_limit, s_byte_limit)
    {}

    void enqueue( const QueuedPacket && p )
    {
        if ( (byte_limit_ && ( queue_size_ + p.contents.size() ) <= byte_limit_ )
            || ( packet_limit_ && queue_size_ < packet_limit_ ) ) {
            queue_size_ += byte_limit_ ? p.contents.size() : 1;
            internal_queue_.emplace( new QueuedPacket( p ) );
        }
    }
};

#endif /* DROP_TAIL_PACKET_QUEUE_HH */ 
