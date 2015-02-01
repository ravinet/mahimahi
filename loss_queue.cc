/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>

#include "loss_queue.hh"
#include "timestamp.hh"

using namespace std;

void LossQueue::read_packet( const string & contents )
{
    bool to_drop = false;
    /* get random number between 0-99, if less than
       loss_percentage then drop packet */
    /* provide seed for random number generator used to create apache pid files */
    srandom( time( NULL ) );
    uint64_t rand_num = rand() % 100;
    if ( rand_num < loss_percentage_ ) { /* drop */
        to_drop = true;
    }

    packet_queue_.emplace( to_drop, contents );
}

void LossQueue::write_packets( FileDescriptor & fd )
{

    while ( !packet_queue_.empty() ) {
        if ( packet_queue_.front().first == false ) {
            fd.write( packet_queue_.front().second );
        }
        packet_queue_.pop();
    }
}

unsigned int LossQueue::wait_time( void ) const
{
    if ( packet_queue_.empty() ) {
        return numeric_limits<uint16_t>::max();
    }

    if ( packet_queue_.front().first == false ) {
        return 0;
    } else {
        return numeric_limits<uint16_t>::max();
    }
}
