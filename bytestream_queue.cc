/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "bytestream_queue.hh"

using namespace std;

void ByteStreamQueue::add( const std::string & buffer )
{
    stream_queue_.emplace( buffer );
}

bool ByteStreamQueue::empty( void )
{
    return stream_queue_.empty();
}
string ByteStreamQueue::head( void ) const
{
    return stream_queue_.front();
}

void ByteStreamQueue::update_head( size_t & amount_written )
{
    if (amount_written == stream_queue_.front().size() ) { /* Head completely written */
        stream_queue_.pop();
    } else { /* Head not completely written */
        stream_queue_.front() = stream_queue_.front().erase( 0, amount_written );
    }
}
