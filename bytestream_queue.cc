/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "bytestream_queue.hh"

using namespace std;

ByteStreamQueue::ByteStreamQueue( const unsigned int size )
    : buffer_ ( size ),
      next_byte_to_read( 0 ),
      next_byte_to_write( 0 )
{
}

unsigned int ByteStreamQueue::available_to_read( void ) const
{
    if ( next_byte_to_read > next_byte_to_write ) { /* wrap around */
        return ( buffer_.size() - next_byte_to_read + next_byte_to_write );
    }
    /* don't wrap around or pointers in same place so return 0 */
    return ( next_byte_to_write - next_byte_to_read );
}

unsigned int ByteStreamQueue::available_to_write ( void ) const
{
    if ( next_byte_to_read > next_byte_to_write ) { /* don't wrap around */
        return (next_byte_to_read - next_byte_to_write - 1 );
    }
    /* wrap around or pointers in same place so return buffer size - 1 */
    return ( buffer_.size() - next_byte_to_write + next_byte_to_read - 1 );
}

void ByteStreamQueue::write( const string & buffer )
{
    /* current position in buffer */
    unsigned int i = 0;

    /* write entire input buffer */
    while ( i < buffer.length() ) {
        buffer_.at( next_byte_to_write ) = buffer.data()[i];
        if ( next_byte_to_write == buffer_.size() - 1 ) { /* wrap around to beginning */
            next_byte_to_write = 0;
        } else {
            next_byte_to_write++;
        }
        i++;
    }
}

void ByteStreamQueue::write_to_fd( FileDescriptor & fd )
{
    unsigned int limit = available_to_read();

    if ( limit == 0 ) { /* nothing available to read */
        return;
    }

    unsigned int current_read_ptr = next_byte_to_read;
    unsigned int j = 0;
    string to_be_written;

    /* make string to be written (of length available_to_read) but don't update actual read pointer yet */
    while ( j < limit ) {
        to_be_written.push_back( buffer_.at( current_read_ptr ) );
        if ( current_read_ptr < buffer_.size() - 1 ) {
            current_read_ptr++;
        } else {
            current_read_ptr = 0;
        }
        j++;
    }

    /* Attempt to write entire constructed string to given fd */
    auto amount_written = fd.writeamount( to_be_written );

    /* Update actual read pointer based on what was written to fd */
    unsigned int update_counter = 0;
    while ( next_byte_to_read < buffer_.size() && update_counter < amount_written ) {
        if ( next_byte_to_read == buffer_.size() - 1 ) {
            next_byte_to_read = 0;
        } else {
            next_byte_to_read++;
        }
        update_counter++;
    }
}
