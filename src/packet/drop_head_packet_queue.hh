/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DROP_HEAD_PACKET_QUEUE_HH
#define DROP_HEAD_PACKET_QUEUE_HH

#include "dropping_packet_queue.hh"

class DropHeadPacketQueue : public DroppingPacketQueue
{
private:
    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "drophead" };
        return type_;
    }

public:
    using DroppingPacketQueue::DroppingPacketQueue;

    void enqueue( QueuedPacket && p ) override
    {
        /* always accept the packet */
        accept( std::move( p ) );

        /* do we need to drop from the head? */
        while ( not good() ) {
            dequeue();
        }
    }
};

#endif /* DROP_HEAD_PACKET_QUEUE_HH */ 
