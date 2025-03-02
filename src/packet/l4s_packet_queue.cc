#include <chrono>

#include "l4s_packet_queue.hh"
#include "timestamp.hh"

using namespace std;

L4SPacketQueue::L4SPacketQueue( const string & args )
  : max_delay_thresh_us_( get_arg( args, "l4s_max_threshold" ) ),
    min_delay_thresh_us_ ( get_arg( args, "l4s_min_threshold" ) ),
    min_qlen_pkt_ ( get_arg( args, "l4s_min_len" ) )
{   
    if ( min_delay_thresh_us_ == 0 ) {
        // Use the step function (as opposed to ramp)
        step_ = true;
    }
    else step_ = false;
        
    if ( max_delay_thresh_us_ == 0 ) 
        max_delay_thresh_us_ = 1200; // us

    if ( min_qlen_pkt_ == 0 )
        min_qlen_pkt_ = 1;
}

uint32_t L4SPacketQueue::calculate_l4s_native_prob ( uint64_t qdelay )
{
    if ( size_packets() <= min_qlen_pkt_ )
        // Do not mark packets if under min_qlen_pkt_ (default is 1)
        return 0;

    // In both the step and the ramp methods:
    if ( qdelay >= max_delay_thresh_us_ ) {
            return MAX_PROB;
        }

    // Here, qdelay < max_delay_thresh_us_
    
    if ( step_ ) {
        return 0;
    }
    else {
        // Use a ramp function: 'laqm (qdelay)' of RFC 9332

        if ( qdelay > min_delay_thresh_us_ ) {
            return scale_prob( ( qdelay - min_delay_thresh_us_ )/
                ( max_delay_thresh_us_ - min_delay_thresh_us_ ) );
        }
        
        return 0;
    }
}

