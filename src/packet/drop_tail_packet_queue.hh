/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DROP_TAIL_PACKET_QUEUE_HH
#define DROP_TAIL_PACKET_QUEUE_HH

#include "dropping_packet_queue.hh"

class DropTailPacketQueue : public DroppingPacketQueue
{
private:
    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "droptail" };
        return type_;
    }

public:
    using DroppingPacketQueue::DroppingPacketQueue;

    void enqueue( QueuedPacket && p ) override
    {
        if ( good_with( size_bytes() + p.contents.size(),
                        size_packets() + 1 ) ) {
            accept( std::move( p ) );
        }

        assert( good() );
    }
};

#endif /* DROP_TAIL_PACKET_QUEUE_HH */ 
