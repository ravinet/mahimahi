#include <math.h>
#include "codel_packet_queue.hh"
#include "timestamp.hh"


using namespace std;

CODELPacketQueue::CODELPacketQueue( const string & args )
  : DroppingPacketQueue(args),
    target_ ( get_arg( args, "target") ),
    interval_ ( get_arg( args, "interval") ),
    first_above_time_ ( 0 ),
    drop_next_( 0 ),
    count_ ( 0 ),
    lastcount_ ( 0 ),
    dropping_ ( 0 )
{
  if ( target_ == 0 || interval_ == 0 ) {
    throw runtime_error( "CoDel queue must have target and interval arguments." );
  }
}

//NOTE: CoDel makes drop decisions at dequeueing. 
//However, this function cannot return NULL. Therefore we ignore
//the drop decision if the current packet is the only one in the queue.
//We know that if this function is called, there is at least one packet in the queue.
dodequeue_result CODELPacketQueue::dodequeue ( uint64_t now )
{
  uint64_t sojourn_time;

  dodequeue_result r;
  r.p = std::move( DroppingPacketQueue::dequeue () );
  r.ok_to_drop = false;

  if ( empty() ) {
    first_above_time_ = 0;
    return r;
  }

  sojourn_time = now - r.p.arrival_time;
  if ( sojourn_time < target_ || size_bytes() <= PACKET_SIZE ) {
    first_above_time_ = 0;
  }
  else {
    if ( first_above_time_ == 0 ) {
      first_above_time_ = now + interval_;
    }
    else if (now >= first_above_time_) {
      r.ok_to_drop = true;
    }
  }

  return r;
}

uint64_t CODELPacketQueue::control_law ( uint64_t t, uint32_t count ) 
{
  double d = interval_ / sqrt (count);
  return t + (uint64_t) d;
}

QueuedPacket CODELPacketQueue::dequeue( void )
{   
  const uint64_t now = timestamp();
  dodequeue_result r = std::move( dodequeue ( now ) );
  uint32_t delta;
    
  if ( dropping_ ) {
    if ( !r.ok_to_drop ) {
      dropping_ = false;
    }

    while ( now >= drop_next_ && dropping_ ) {
      dodequeue_result r = std::move( dodequeue ( now ) );
      count_++;
      if ( ! r.ok_to_drop ) {
	dropping_ = false;
      } else {
	drop_next_ = control_law(drop_next_, count_);
      }
    }
  }
  else if ( r.ok_to_drop ) {
    dodequeue_result r = std::move( dodequeue ( now ) );
    dropping_ = true;
    delta = count_ - lastcount_;
    count_ = ( ( delta > 1 ) && ( now - drop_next_ < 16 * interval_ ))? 
      delta : 1;
    drop_next_ = control_law ( now, count_ );
    lastcount_ = count_;
  }

  return r.p;
}


void CODELPacketQueue::enqueue( QueuedPacket && p )
{
  if ( good_with( size_bytes() + p.contents.size(),
		  size_packets() + 1 ) ) {
    accept( std::move( p ) );
  }
  assert( good() );
}
