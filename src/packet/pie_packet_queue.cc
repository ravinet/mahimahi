
#include <chrono>

#include "pie_packet_queue.hh"
#include "timestamp.hh"

using namespace std;

#define DQ_COUNT_INVALID   (uint32_t)-1

PIEPacketQueue::PIEPacketQueue( const string & args )
  : DroppingPacketQueue(args),
    qdelay_ref_ ( get_arg( args, "qdelay_ref" ) ),
    max_burst_ ( get_arg( args, "max_burst" ) ),
    alpha_ ( 0.125 ),
    beta_ ( 1.25 ),
    t_update_ ( 30 ),
    dq_threshold_ ( 16384 ),
    drop_prob_ ( 0.0 ),
    burst_allowance_ ( 0 ),
    qdelay_old_ ( 0 ),
    current_qdelay_ ( 0 ),
    dq_count_ ( DQ_COUNT_INVALID ),
    dq_tstamp_ ( 0 ),
    avg_dq_rate_ ( 0 ),
    uniform_generator_ ( 0.0, 1.0 ),
    prng_( random_device()() ),
    last_update_( timestamp() )
{
  if ( qdelay_ref_ == 0 || max_burst_ == 0 ) {
    throw runtime_error( "PIE AQM queue must have qdelay_ref and max_burst parameters" );
  }
}

void PIEPacketQueue::enqueue( QueuedPacket && p )
{
  calculate_drop_prob();

  if ( ! good_with( size_bytes() + p.contents.size(),
		    size_packets() + 1 ) ) {
    // Internal queue is full. Packet has to be dropped.
    return;
  } 

  if (!drop_early() ) {
    //This is the negation of the pseudo code in the IETF draft.
    //It is used to enqueue rather than drop the packet
    //All other packets are dropped
    accept( std::move( p ) );
  }

  assert( good() );
}

//returns true if packet should be dropped.
bool PIEPacketQueue::drop_early ()
{
  if ( burst_allowance_ > 0 ) {
    return false;
  }

  if ( qdelay_old_ < qdelay_ref_/2 && drop_prob_ < 0.2) {
    return false;        
  }

  if ( size_bytes() < (2 * PACKET_SIZE) ) {
    return false;
  }

  double random = uniform_generator_(prng_);

  if ( random < drop_prob_ ) {
    return true;
  }
  else
    return false;
}

QueuedPacket PIEPacketQueue::dequeue( void )
{
  QueuedPacket ret = std::move( DroppingPacketQueue::dequeue () );
  uint32_t now = timestamp();

  if ( size_bytes() >= dq_threshold_ && dq_count_ == DQ_COUNT_INVALID ) {
    dq_tstamp_ = now;
    dq_count_ = 0;
  }

  if ( dq_count_ != DQ_COUNT_INVALID ) {
    dq_count_ += ret.contents.size();

    if ( dq_count_ > dq_threshold_ ) {
      uint32_t dtime = now - dq_tstamp_;

      if ( dtime > 0 ) {
	uint32_t rate_sample = dq_count_ / dtime;
	if ( avg_dq_rate_ == 0 ) 
	  avg_dq_rate_ = rate_sample;
	else
	  avg_dq_rate_ = ( avg_dq_rate_ - (avg_dq_rate_ >> 3 )) +
		     (rate_sample >> 3);
                
	if ( size_bytes() < dq_threshold_ ) {
	  dq_count_ = DQ_COUNT_INVALID;
	}
	else {
	  dq_count_ = 0;
	  dq_tstamp_ = now;
	} 

	if ( burst_allowance_ > 0 ) {
	  if ( burst_allowance_ > dtime )
	    burst_allowance_ -= dtime;
	  else
	    burst_allowance_ = 0;
	}
      }
    }
  }

  calculate_drop_prob();

  return ret;
}

void PIEPacketQueue::calculate_drop_prob( void )
{
  uint64_t now = timestamp();

  //We can't have a fork inside the mahimahi shell so we simulate
  //the periodic drop probability calculation here by repeating it for the
  //number of periods missed since the last update. 
  //In the interval [last_update_, now] no change occured in queue occupancy 
  //so when this value is used (at enqueue) it will be identical
  //to that of a timer-based drop probability calculation.
  while (now - last_update_ > t_update_) {
    bool update_prob = true;
    qdelay_old_ = current_qdelay_;

    if ( avg_dq_rate_ > 0 ) 
      current_qdelay_ = size_bytes() / avg_dq_rate_;
    else
      current_qdelay_ = 0;

    if ( current_qdelay_ == 0 && size_bytes() != 0 ) {
      update_prob = false;
    }

    double p = (alpha_ * (int)(current_qdelay_ - qdelay_ref_) ) +
      ( beta_ * (int)(current_qdelay_ - qdelay_old_) );

    if ( drop_prob_ < 0.01 ) {
      p /= 128;
    } else if ( drop_prob_ < 0.1 ) {
      p /= 32;
    } else  {
      p /= 16;
    } 

    drop_prob_ += p;

    if ( drop_prob_ < 0 ) {
      drop_prob_ = 0;
    }
    else if ( drop_prob_ > 1 ) {
      drop_prob_ = 1;
      update_prob = false;
    }

        
    if ( current_qdelay_ == 0 && qdelay_old_==0 && update_prob) {
      drop_prob_ *= 0.98;
    }
        
    burst_allowance_ = max( 0, (int) burst_allowance_ -  (int)t_update_ );
    last_update_ += t_update_;

    if ( ( drop_prob_ == 0 )
	 && ( current_qdelay_ < qdelay_ref_/2 ) 
	 && ( qdelay_old_ < qdelay_ref_/2 ) 
	 && ( avg_dq_rate_ > 0 ) ) {
      dq_count_ = DQ_COUNT_INVALID;
      avg_dq_rate_ = 0;
      burst_allowance_ = max_burst_;
    }

  }
}
