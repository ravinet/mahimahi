/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DROPPING_PACKET_QUEUE_HH
#define DROPPING_PACKET_QUEUE_HH

#include <deque>
#include <cassert>

#include "abstract_packet_queue.hh"

class DroppingPacketQueue : public AbstractPacketQueue
{
private:
    int queue_size_in_bytes_ = 0, queue_size_in_packets_ = 0;

    std::queue<QueuedPacket> internal_queue_ {};

protected:
    const unsigned int packet_limit_;
    const unsigned int byte_limit_;

    unsigned int size_bytes( void ) const
    {
        assert( queue_size_in_bytes_ >= 0 );
        return unsigned( queue_size_in_bytes_ );
    }

    unsigned int size_packets( void ) const
    {
        assert( queue_size_in_packets_ >= 0 );
        return unsigned( queue_size_in_packets_ );
    }

    /* put a packet on the back of the queue */
    void accept( QueuedPacket && p )
    {
        queue_size_in_bytes_ += p.contents.size();
        queue_size_in_packets_++;
        internal_queue_.emplace( std::move( p ) );
    }

    /* are the limits currently met? */
    bool good( void ) const
    {
        bool ret = true;

        if ( byte_limit_ ) {
            ret &= ( size_bytes() <= byte_limit_ );
        }

        if ( packet_limit_ ) {
            ret &= ( size_packets() <= packet_limit_ );
        }

        return ret;
    }

public:
    DroppingPacketQueue( const unsigned int packet_limit, const unsigned int byte_limit )
        : packet_limit_( packet_limit ), byte_limit_( byte_limit )
    {}

    virtual void enqueue( QueuedPacket && p ) = 0;

    QueuedPacket dequeue( void ) override
    {
        assert( not internal_queue_.empty() );

        QueuedPacket ret = std::move( internal_queue_.front() );
        internal_queue_.pop();

        queue_size_in_bytes_ -= ret.contents.size();
        queue_size_in_packets_--;

        assert( good() );

        return ret;
    }

    bool empty( void ) const override
    {
        return internal_queue_.empty();
    }
};

#endif /* DROPPING_PACKET_QUEUE_HH */ 
