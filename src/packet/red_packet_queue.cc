#include "red_packet_queue.hh"
#include <algorithm>
#include "timestamp.hh"

using namespace std;

REDPacketQueue::REDPacketQueue( ParsedArguments & args)
  : DroppingPacketQueue(args),
    wq_(args.get_float_arg("wq")),
    min_thresh_(args.get_float_arg("minthresh")),
    max_thresh_(args.get_float_arg("maxthresh")),
    transmission_time_(args.get_int_arg("transmission_time")),
    time_at_zero_q_(0),
    prng_( random_device()() ),
    drop_dist_ (0, 1),
    current_random_val_(0),
    count_(0)
{

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
    auto instantaneous_queue_size = size_packets();

    /* If the queue is empty, decay the weighted average by
     * the amount of time that the queue has been
     * empty. */
    if (size_packets() == 0) {
        weighted_average_ = powf((1 - wq_), (timestamp() - time_at_zero_q_)/transmission_time_) * weighted_average_;
    } else {
        weighted_average_ = (instantaneous_queue_size * wq_ ) + (1- wq_) * weighted_average_;
    }


    /* ratio is how full the queue is percentage-wise */
    auto ratio = (weighted_average_)/packet_limit_;
    bool accept_packet = true;

    /*
     * Logic for enqueuing packets:
     *  - If ratio is between the min and max threshold,
     *  begin counting the number of packets that have
     *  passed through the queue. We compute a drop
     *  probability based on `ratio`, and if the
     *  number of packets in the range exceeds
     *  some random number divided by that drop probability,
     *  drop the packet, and reset teh count.
     *  - If ratio is greater than the max_thresh_, drop
     * the packet.
     *  - If it is under max thresh, don't
     *  drop the packet.
     *  - See: http://www.icir.org/floyd/papers/early.twocolumn.pdf
     *  for more details
     */
    if (ratio >= min_thresh_ && ratio <= max_thresh_) {
      count_++;
      float drop_probability = (ratio - min_thresh_)/(max_thresh_ - min_thresh_);

      if (count_ > 0 && count_ >= current_random_val_/drop_probability ) {
        accept_packet = false;
        count_ = 0;
      }

      if (count_ == 0) {
        current_random_val_ = drop_dist_(prng_);
      }
    } else if (ratio > max_thresh_ ) {
      accept_packet = false;
      count_ = 0;
    } else {
      count_ = -1;
    }

    if ( accept_packet && good_with( size_bytes() + p.contents.size(),
                    size_packets() + 1 ) ) {
        accept( move( p ) );
    }

    assert( good() );
}
