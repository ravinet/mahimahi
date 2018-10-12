/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef INFINITE_PACKET_QUEUE_HH
#define INFINITE_PACKET_QUEUE_HH

#include <queue>
#include <cassert>

#include "queued_packet.hh"
#include "abstract_packet_queue.hh"
#include "exception.hh"

class InfinitePacketQueue : public AbstractPacketQueue
{
private:
    std::queue<QueuedPacket> internal_queue_ {};
    int queue_size_in_bytes_ = 0, queue_size_in_packets_ = 0;

public:
    InfinitePacketQueue( const std::string & args )
    {
        if ( not args.empty() ) {
            throw std::runtime_error( "InfinitePacketQueue does not take arguments." );
        }
    }

    void enqueue( QueuedPacket && p ) override
    {
        queue_size_in_bytes_ += p.contents.size();
        queue_size_in_packets_++;
        internal_queue_.emplace( std::move( p ) );
    }

    QueuedPacket dequeue( void ) override
    {
        assert( not internal_queue_.empty() );

        QueuedPacket ret = std::move( internal_queue_.front() );
        internal_queue_.pop();

        queue_size_in_bytes_ -= ret.contents.size();
        queue_size_in_packets_--;

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

    unsigned int size_bytes( void ) const override
    {
        assert( queue_size_in_bytes_ >= 0 );
        return unsigned( queue_size_in_bytes_ );
    }

    unsigned int size_packets( void ) const override
    {
        assert( queue_size_in_packets_ >= 0 );
        return unsigned( queue_size_in_packets_ );
    }

};

#endif /* INFINITE_PACKET_QUEUE_HH */ 
