/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <queue>
#include <assert.h>
#include "finite_queue.hh"

using namespace std;

FiniteQueue::FiniteQueue( signed int max_size )
    : finite_queue_(),
      queue_size_(0),
      max_size_( max_size )
    {}

FiniteQueue::QueuedPacket::QueuedPacket( const std::string & s_contents )
    : bytes_to_transmit( s_contents.size() ),
      contents( s_contents )
{}

void FiniteQueue::emplace( const std::string & contents )
{
    if ( max_size_ - queue_size_  > contents.size() ) {
        finite_queue_.emplace( contents );
        queue_size_ = queue_size_ + contents.size();
        assert( queue_size_ <= max_size_ );
    }
}   

bool FiniteQueue::empty( void ) const
{
    return finite_queue_.empty();
}

FiniteQueue::QueuedPacket & FiniteQueue::front( void )
{
    return finite_queue_.front();
}

void FiniteQueue::pop( void )
{
    queue_size_ = queue_size_ - front().contents.size();
    finite_queue_.pop();
}
