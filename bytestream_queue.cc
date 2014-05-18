/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cassert>

#include "bytestream_queue.hh"

using namespace std;

ByteStreamQueue::ByteStreamQueue( const size_t size )
    : buffer_( size, 0 ),
      next_byte_to_push( 0 ),
      next_byte_to_pop( 0 ),
      space_available( [&] () { return available_to_push() > 0; } ),
      non_empty( [&] () { return available_to_pop() > 0; } )
{
    assert( size > 1 );
}

size_t ByteStreamQueue::available_to_pop( void ) const
{
    return next_byte_to_push - next_byte_to_pop
        + ( next_byte_to_pop > next_byte_to_push ? buffer_.size() : 0 );
}

size_t ByteStreamQueue::available_to_push( void ) const
{
    return next_byte_to_pop - next_byte_to_push - 1
        + ( next_byte_to_pop > next_byte_to_push ? 0 : buffer_.size() );
}

ByteStreamQueue::Result ByteStreamQueue::push( FileDescriptor & fd )
{
    /* will need to handle case if it's possible that more than
       one action might push to this queue */
    assert( space_available() );

    size_t contiguous_space_to_push = available_to_push();
    if ( next_byte_to_push + contiguous_space_to_push >= buffer_.size() ) {
        contiguous_space_to_push = buffer_.size() - next_byte_to_push;
    }

    assert( contiguous_space_to_push > 0 );

    /* read from the fd */
    string new_chunk = fd.read( contiguous_space_to_push );
    if ( new_chunk.empty() ) {
        return Result::EndOfFile;
    }

    assert( new_chunk.size() <= buffer_.size() - next_byte_to_push );
    memcpy( &buffer_.at( next_byte_to_push ), new_chunk.data(), new_chunk.size() );

    next_byte_to_push += new_chunk.size();
    assert( next_byte_to_push <= buffer_.size() );
    if ( next_byte_to_push == buffer_.size() ) {
        next_byte_to_push = 0;
        }

    assert( non_empty() );
    return Result::Success;
}

size_t ByteStreamQueue::contiguous_space_to_push( void )
{
    size_t contiguous_space_to_push = available_to_push();
    if ( next_byte_to_push + contiguous_space_to_push >= buffer_.size() ) {
        contiguous_space_to_push = buffer_.size() - next_byte_to_push;
    }
    //assert( contiguous_space_to_push > 0 );
    return contiguous_space_to_push;
}

ByteStreamQueue::Result ByteStreamQueue::push_string( const string & new_chunk )
{
    /* will need to handle case if it's possible that more than
       one action might push to this queue */
    assert( space_available() );

    if ( new_chunk.empty() ) {
        return Result::EndOfFile;
    }

    assert( new_chunk.size() <= buffer_.size() - next_byte_to_push );
    memcpy( &buffer_.at( next_byte_to_push ), new_chunk.data(), new_chunk.size() );

    next_byte_to_push += new_chunk.size();
    assert( next_byte_to_push <= buffer_.size() );

    if ( next_byte_to_push == buffer_.size() ) {
        next_byte_to_push = 0;
    }
    assert( non_empty() );
    return Result::Success;
}

void ByteStreamQueue::pop( FileDescriptor & fd )
{
    /* will need to handle case if it's possible that more than
       one action might pop from this queue */
    assert( non_empty() );

    size_t contiguous_space_to_pop = available_to_pop();
    if ( next_byte_to_pop + contiguous_space_to_pop >= buffer_.size() ) {
        contiguous_space_to_pop = buffer_.size() - next_byte_to_pop;
    }
    
    decltype(buffer_)::const_iterator pop_iterator = buffer_.begin() + next_byte_to_pop;
    auto end_of_pop = pop_iterator + contiguous_space_to_pop;

    assert( end_of_pop >= pop_iterator );

    pop_iterator = fd.write_some( pop_iterator, end_of_pop );

    next_byte_to_pop = pop_iterator - buffer_.begin();
    assert( next_byte_to_pop <= buffer_.size() );
    if ( next_byte_to_pop == buffer_.size() ) {
        next_byte_to_pop = 0;
    }
}

void ByteStreamQueue::pop_ssl( unique_ptr<ReadWriteInterface> && rw )
{
    /* will need to handle case if it's possible that more than
       one action might pop from this queue */
    assert( non_empty() );

    size_t contiguous_space_to_pop = available_to_pop();
    if ( next_byte_to_pop + contiguous_space_to_pop >= buffer_.size() ) {
        contiguous_space_to_pop = buffer_.size() - next_byte_to_pop;
    }

    decltype(buffer_)::const_iterator pop_iterator = buffer_.begin() + next_byte_to_pop;
    auto end_of_pop = pop_iterator + contiguous_space_to_pop;

    assert( end_of_pop >= pop_iterator );

    pop_iterator = rw->write_some( pop_iterator, end_of_pop );

    next_byte_to_pop = pop_iterator - buffer_.begin();
    assert( next_byte_to_pop <= buffer_.size() );
    if ( next_byte_to_pop == buffer_.size() ) {
        next_byte_to_pop = 0;
    }
}

bool eof( const ByteStreamQueue::Result & r )
{
    return r == ByteStreamQueue::Result::EndOfFile;
}
