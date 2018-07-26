#include "red_packet_queue.hh"
#include <algorithm>
#include "timestamp.hh"

using namespace std;

REDPacketQueue::REDPacketQueue( const string & args)
  : DroppingPacketQueue(args),
    wq_(get_float_arg(args, "wq")),
    min_thresh_(get_float_arg(args, "minthresh")),
    max_thresh_(get_float_arg(args, "maxthresh")),
    time_at_zero_q_(0)
{
  if (packet_limit_ == 0) {
    throw runtime_error( "RED queue must have packet limit." );
  }

  if ( wq_ == 0.0 || min_thresh_ == 0.0 || max_thresh_ == 0.0 ) {
    throw runtime_error( "RED queue must have wq, minthresh, and maxthresh arguments." );
  }

}

QueuedPacket REDPacketQueue::dequeue( void )
{
    auto packet = DroppingPacketQueue::dequeue();
    if (size_packets() == 0) {
      time_at_zero_q_ = timestamp();
    }

    return packet;
}

void REDPacketQueue::enqueue( QueuedPacket && p )
{
    int s = 4;
    auto instantaneous_queue_size = size_packets();

    /* If the queue is empty, decay the weighted average by
     * the amount of time that the queue has been
     * empty. */
    if (size_packets() == 0) {
        weighted_average_ = powf((1 - wq_), (timestamp() - time_at_zero_q_)/s) * weighted_average_;
    } else {
        weighted_average_ = (instantaneous_queue_size * wq_ ) + (1- wq_) * weighted_average_;
    }

    auto ratio = (weighted_average_)/packet_limit_;
    std::default_random_engine generator (0);
    std::uniform_real_distribution<double> distribution (min_thresh_, max_thresh_);
    double threshold = distribution(generator);
    if (ratio > max_thresh_) {
      ratio = 1;
    }
    if (ratio < min_thresh_) {
      ratio = 0;
    }

    if ( (threshold > ratio) && good_with( size_bytes() + p.contents.size(),
                    size_packets() + 1 ) ) {
        accept( std::move( p ) );
    }

    assert( good() );
}
