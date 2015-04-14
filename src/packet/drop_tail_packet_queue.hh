/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DROP_TAIL_PACKET_QUEUE_HH
#define DROP_TAIL_PACKET_QUEUE_HH

#include "dropping_packet_queue.hh"

class DropTailPacketQueue : public DroppingPacketQueue
{
public:
    using DroppingPacketQueue::DroppingPacketQueue;

    void enqueue( QueuedPacket && p ) override
    {
        bool accept_packet = true;

        if ( byte_limit_ ) {
            accept_packet &= ( size_bytes() + p.contents.size() <= byte_limit_ );
        }

        if ( packet_limit_ ) {
            accept_packet &= ( size_packets() + 1 <= packet_limit_ );
        }

        if ( accept_packet ) {
            accept( std::move( p ) );
        }

        assert( good() );
    }
};

#endif /* DROP_TAIL_PACKET_QUEUE_HH */ 
