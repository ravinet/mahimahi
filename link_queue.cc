/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>
#include <fstream>

#include "link_queue.hh"
#include "timestamp.hh"
#include "util.hh"
#include "ezio.hh"

using namespace std;

LinkQueue::LinkQueue( const std::string & filename )
    : next_delivery_( 0 ),
      schedule_(),
      base_timestamp_( timestamp() ),
      packet_queue_()
{
    assert_not_root();

    /* open filename and load schedule */
    ifstream trace_file( filename );

    string line;

    while ( getline( trace_file, line ) ) {
        if ( line.empty() ) {
            throw Exception( filename, "invalid empty line" );
        }

        const uint64_t ms = myatoi( line );

        if ( not schedule_.empty() ) {
            if ( ms < schedule_.back() ) {
                throw Exception( filename, "timestamps must be monotonically nondecreasing" );
            }
        }

        schedule_.emplace_back( ms );
    }

    if ( schedule_.empty() ) {
        throw Exception( filename, "no valid timestamps found" );
    }
}

LinkQueue::QueuedPacket::QueuedPacket( const std::string & s_contents )
    : bytes_to_transmit( s_contents.size() ),
      contents( s_contents )
{}

void LinkQueue::read_packet( const string & contents )
{
    packet_queue_.emplace( contents );
}

uint64_t LinkQueue::next_delivery_time( void ) const
{
    return schedule_.at( next_delivery_ ) + base_timestamp_;
}

void LinkQueue::use_a_delivery_opportunity( void )
{
    next_delivery_ = (next_delivery_ + 1) % schedule_.size();

    /* wraparound */
    if ( next_delivery_ == 0 ) {
        base_timestamp_ += schedule_.back();
    }
}

void LinkQueue::write_packets( FileDescriptor & fd )
{
    uint64_t now = timestamp();

    while ( next_delivery_time() <= now ) {
        /* burn a delivery opportunity */
        unsigned int bytes_left_in_this_delivery = PACKET_SIZE;
        use_a_delivery_opportunity();

        while ( (bytes_left_in_this_delivery > 0)
                and (not packet_queue_.empty()) ) {
            packet_queue_.front().bytes_to_transmit -= bytes_left_in_this_delivery;
            bytes_left_in_this_delivery = 0;

            if ( packet_queue_.front().bytes_to_transmit <= 0 ) {
                /* restore the surplus bytes beyond what the packet requires */
                bytes_left_in_this_delivery += (- packet_queue_.front().bytes_to_transmit);

                /* this packet is ready to go */
                fd.write( packet_queue_.front().contents );
                packet_queue_.pop();
            }
        }
    }
}

unsigned int LinkQueue::wait_time( void ) const
{
    if ( packet_queue_.empty() ) {
        return numeric_limits<uint16_t>::max();
    }

    const auto now = timestamp();

    if ( next_delivery_time() <= now ) {
        return 0;
    } else {
        return next_delivery_time() - now;
    }
}
