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
    if ( next_byte_to_read == 0 && next_byte_to_write == 0 ) { /* beginning or nothing to read */
        return 0;
    }
    if ( next_byte_to_read > next_byte_to_write ) { /* must wrap around */
        return ( buffer_.size() - next_byte_to_read + next_byte_to_write );
    } else { /* write pointer in front of read pointer so don't wrap around */
        return ( next_byte_to_write - next_byte_to_read );
    }
}

unsigned int ByteStreamQueue::available_to_write ( void ) const
{
    if ( next_byte_to_read == 0 && next_byte_to_write == 0 ) { /* beginning or just empty */
        return buffer_.size();
    }
    if ( next_byte_to_write > next_byte_to_read ) { /* must wrap around */
        return ( buffer_.size() - next_byte_to_write + next_byte_to_read );
    } else { /* read pointer in front of write pointer so don't wrap around */
        return ( next_byte_to_read - next_byte_to_write );
    }
}

void ByteStreamQueue::add( const string & buffer )
{
    /* Must check if buffer length is more than available_to_write (if so, don't write) */

    /* current position in buffer */
    unsigned int i = 0;
    /* write until write pointer is at end of vector or you have written entire string */
    while ( next_byte_to_write < buffer_.size() && i < buffer.length() ) {
        buffer_.at( next_byte_to_write ) = buffer.data()[i];
        next_byte_to_write++;
        i++;
    }
    /* Must reset next_byte_to_write to front and continue until end of string*/
    next_byte_to_write = 0;
    while ( i < buffer.length() ) {
        buffer_.at( next_byte_to_write ) = buffer.data()[i];
        next_byte_to_write++;
        i++;
    }
}

void ByteStreamQueue::write_to_fd( FileDescriptor & fd )
{
    unsigned int limit = available_to_write();
    unsigned int j = 0;
    string to_be_written;

    if ( limit == 0 ) { /* nothing available to read */
        return;
    }
    unsigned int current_read_ptr = next_byte_to_read;
    /* Make string to be written (of length available to read) but don't update read pointer */
    while ( current_read_ptr < buffer_.size() && j < limit ) {
        to_be_written.push_back( buffer_.at( current_read_ptr ) );
        current_read_ptr++;
        j++;
    }

    /* set current_read_ptr to beginning and continue*/
    current_read_ptr = 0;
    while ( j < limit ) {
        to_be_written.push_back( buffer_.at( current_read_ptr ) );
        current_read_ptr++;
        j++;
    }

    /* Write constructed string to given file descriptor */
    auto amount_written = fd.writeamount( to_be_written );

    /* Update next_byte_to_read based on what was written */
    unsigned int update_counter = 0;
    while ( next_byte_to_read < buffer_.size() && update_counter < amount_written ) {
        next_byte_to_read++;
        update_counter++;
    }
    /* set next_byte_to_read to beginning and continue */
    next_byte_to_read = 0;
    while ( update_counter < amount_written ) {
        next_byte_to_read++;
        update_counter++;
    }
}
