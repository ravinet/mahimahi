/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>
#include <cinttypes>
#include <cstdio>

#include "link_queue.hh"
#include "timestamp.hh"
#include "util.hh"

using namespace std;

LinkQueue::LinkQueue( const std::string & filename, const int & max_size )
    : next_delivery_( 0 ),
      schedule_(),
      base_timestamp_( timestamp() ),
      packet_queue_( max_size )
{
    assert_not_root();

    /* open filename and load schedule */
    FileDescriptor trace_file( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );
    FILE *f = fdopen( trace_file.num(), "r" );
    if ( f == nullptr ) {
        throw Exception( "fopen" );
    }

    while ( true ) {
        uint64_t ms;
        int num_matched = fscanf( f, "%" PRIu64 "\n", &ms );
        if ( num_matched != 1 ) {
            break;
        }

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
