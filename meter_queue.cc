/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>

#include "meter_queue.hh"
#include "util.hh"

using namespace std;

MeterQueue::MeterQueue( const bool graph )
    : packet_queue_(),
      graph_( graph )
{
    assert_not_root();
}

void MeterQueue::read_packet( const string & contents )
{
    packet_queue_.emplace( contents );

    /* graph it */
    if ( graph_ ) {
        //
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
