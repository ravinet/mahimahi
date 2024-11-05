/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include "dropping_packet_queue.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

DroppingPacketQueue::DroppingPacketQueue( ParsedArguments & args )
    : packet_limit_( args.get_int_arg( "packets", 0 ) ),
      byte_limit_( args.get_int_arg( "bytes", 0 ) )
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

bool DroppingPacketQueue::good_with( const unsigned int size_in_bytes,
                                     const unsigned int size_in_packets ) const
{
    bool ret = true;

    if ( byte_limit_ ) {
        ret &= ( size_in_bytes <= byte_limit_ );
    }

    if ( packet_limit_ ) {
        ret &= ( size_in_packets <= packet_limit_ );
    }

    return ret;
}

bool DroppingPacketQueue::good( void ) const
{
    return good_with( size_bytes(), size_packets() );
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

string DroppingPacketQueue::to_string( void ) const
{
    string ret = type() + " [";

    if ( byte_limit_ ) {
        ret += string( "bytes=" ) + ::to_string( byte_limit_ );
    }

    if ( packet_limit_ ) {
        if ( byte_limit_ ) {
            ret += ", ";
        }

        ret += string( "packets=" ) + ::to_string( packet_limit_ );
    }

    ret += "]";

    return ret;
}
