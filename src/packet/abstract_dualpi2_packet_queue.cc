#include <chrono>

#include "abstract_dualpi2_packet_queue.hh"
#include "dropping_packet_queue.hh"
#include "timestamp.hh"

using namespace std;

#define DQ_COUNT_INVALID   (uint32_t)-1

void AbstractDualPI2PacketQueue::enqueue( QueuedPacket && p )
{
    queue_size_in_bytes_ += p.contents.size();
    queue_size_in_packets_++;
    internal_queue_.emplace( std::move( p ) );
}

QueuedPacket AbstractDualPI2PacketQueue::dequeue( void )
{
    assert( not internal_queue_.empty() );

    QueuedPacket ret = std::move( internal_queue_.front() );
    internal_queue_.pop();

    queue_size_in_bytes_ -= ret.contents.size();
    queue_size_in_packets_--;

    return ret;
}

bool AbstractDualPI2PacketQueue::empty( void ) const
{
    return internal_queue_.empty();
}

unsigned int AbstractDualPI2PacketQueue::size_bytes( void ) const
{
    assert( queue_size_in_bytes_ >= 0 );
    return unsigned( queue_size_in_bytes_ );
}

unsigned int AbstractDualPI2PacketQueue::size_packets( void ) const
{
    assert( queue_size_in_packets_ >= 0 );
    return unsigned( queue_size_in_packets_ );
}



string AbstractDualPI2PacketQueue::to_string( void ) const
{
    // string ret = type() + " [";

    // if ( byte_limit_ ) {
    //     ret += string( "bytes=" ) + ::to_string( byte_limit_ );
    // }

    // if ( packet_limit_ ) {
    //     if ( byte_limit_ ) {
    //         ret += ", ";
    //     }

    //     ret += string( "packets=" ) + ::to_string( packet_limit_ );
    // }

    // ret += "]";

    return "";
}

QueuedPacket& AbstractDualPI2PacketQueue::peek( void ) 
{
    return internal_queue_.front();
}

uint64_t AbstractDualPI2PacketQueue::qdelay_in_ns ( uint64_t ref ) 
{
    if ( internal_queue_.empty() ) return 0;
    
    QueuedPacket& head = peek();
    return head.sojourn_time_in_ns( ref );
}

uint32_t scale_prob( double prob )
{
    if ( prob < 0.0 || prob > 1.0 )
        throw runtime_error ("Probability out of range! Provided value: " + std::to_string(prob));

    return static_cast<uint32_t> ( prob * MAX_PROB ) ;
}

unsigned int get_arg( const string & args, const string & name )
{
    std::cout << " Got into get_arg!!!\n";
    return DroppingPacketQueue::get_arg( args, name );
}

