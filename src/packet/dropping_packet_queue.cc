/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include "dropping_packet_queue.hh"
#include "exception.hh"

using namespace std;

static unsigned int get_arg( const string & args, const string & name )
{
    const auto offset = args.find( name );
    if ( offset == string::npos ) {
        return 0; /* default value */
    } else {
        /* extract the value */
        return stoul( args.substr( offset + name.size() + 1 ) );
    }
}

DroppingPacketQueue::DroppingPacketQueue( const string & args )
    : packet_limit_( get_arg( args, "packets" ) ),
      byte_limit_( get_arg( args, "bytes" ) )
{
    if ( packet_limit_ == 0 and byte_limit_ == 0 ) {
        throw runtime_error( "Dropping queue must have a byte or packet limit." );
    }
}

QueuedPacket DroppingPacketQueue::dequeue( void )
{
    assert( not internal_queue_.empty() );

    QueuedPacket ret = std::move( internal_queue_.front() );
    internal_queue_.pop();

    queue_size_in_bytes_ -= ret.contents.size();
    queue_size_in_packets_--;

    assert( good() );

    return ret;
}

bool DroppingPacketQueue::empty( void ) const
{
    return internal_queue_.empty();
}

bool DroppingPacketQueue::good( void ) const
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

unsigned int DroppingPacketQueue::size_bytes( void ) const
{
    assert( queue_size_in_bytes_ >= 0 );
    return unsigned( queue_size_in_bytes_ );
}

unsigned int DroppingPacketQueue::size_packets( void ) const
{
    assert( queue_size_in_packets_ >= 0 );
    return unsigned( queue_size_in_packets_ );
}

/* put a packet on the back of the queue */
void DroppingPacketQueue::accept( QueuedPacket && p )
{
    queue_size_in_bytes_ += p.contents.size();
    queue_size_in_packets_++;
    internal_queue_.emplace( std::move( p ) );
}
