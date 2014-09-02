/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>
#include <cassert>
#include <thread>
#include <cmath>

#include "meter_queue.hh"
#include "util.hh"
#include "timestamp.hh"

using namespace std;

MeterQueue::MeterQueue( const string & name, const bool graph )
    : packet_queue_(),
      graph_( nullptr ),
      bytes_this_bin_( 0 ),
      bin_width_( 250 ),
      current_bin_( timestamp() / bin_width_ ),
      logical_width_( graph ? max( 5.0, 640 / 100.0 ) : 1 ),
      logical_height_( 1 )
{
    assert_not_root();

    if ( graph ) {
        graph_.reset( new Graph( 1, 640, 480, name, 0, 1 ) );
        graph_->set_color( 0, 1, 0, 0, 0.5 );
        thread newthread( [&] () {
                while ( true ) {
                    const uint64_t ts = advance();

                    double current_estimate = (bytes_this_bin_ * 8.0 / (bin_width_ / 1000.0)) / 1000000.0;
                    double bin_fraction = (ts % bin_width_) / double( bin_width_ );
                    double confidence = 1 - cos( bin_fraction * 3.14159 / 2.0 );

                    if ( confidence > 0.75 and (current_estimate / bin_fraction) > (logical_height_ * 0.85) ) {
                        logical_height_ = (current_estimate / bin_fraction) * 1.2;
                    }

                    graph_->blocking_draw( ts / 1000.0, logical_width_, 0, logical_height_,
                                           { float( current_estimate / bin_fraction ) },
                                           1 - cos( bin_fraction * 3.14159 / 2.0 ) );
                }
            } );
        newthread.detach();
    }
}

uint64_t MeterQueue::advance( void )
{
    assert( graph_ );

    const uint64_t now = timestamp();

    const uint64_t now_bin = now / bin_width_;

    if ( current_bin_ == now_bin ) {
        return now;
    }

    while ( current_bin_ < now_bin ) {
        graph_->add_data_point( 0,
                                (current_bin_ + 1) * bin_width_ / 1000.0,
                                (bytes_this_bin_ * 8.0 / (bin_width_ / 1000.0)) / 1000000.0 );
        bytes_this_bin_ = 0;
        current_bin_++;
    }

    return now;
}

void MeterQueue::read_packet( const string & contents )
{
    packet_queue_.emplace( contents );

    /* save it */
    if ( graph_ ) {
        advance();
        bytes_this_bin_ += contents.size();
    }
}

void MeterQueue::write_packets( FileDescriptor & fd )
{
    while ( not packet_queue_.empty() ) {
        fd.write( packet_queue_.front() );
        packet_queue_.pop();
    }
}

unsigned int MeterQueue::wait_time( void ) const
{
    return packet_queue_.empty() ? numeric_limits<uint16_t>::max() : 0;
}
