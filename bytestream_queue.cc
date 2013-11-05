/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "bytestream_queue.hh"

using namespace std;

ByteStreamQueue::ByteStreamQueue( const unsigned int size )
    : buffer_ ( size ),
      next_byte_to_read( 0 ),
      next_byte_to_write( 0 )
{
}

/* How much space available to be read from bytestreamqueue and written to fd */
unsigned int ByteStreamQueue::available_to_read( void ) const
{
    if ( next_byte_to_read > next_byte_to_write ) { /* must wrap around */
        return ( buffer_.size() - next_byte_to_read + next_byte_to_write );
    } else if ( next_byte_to_write > next_byte_to_read ) { /* write pointer in front of read pointer so don't wrap around */
        return ( next_byte_to_write - next_byte_to_read );
    } else { /* read and write pointers are at same place */
        return 0;
    }
}

/* How much space available to write to bytestreamqueue */
unsigned int ByteStreamQueue::available_to_write ( void ) const
{
    if ( next_byte_to_write > next_byte_to_read ) { /* must wrap around */
        return ( buffer_.size() - next_byte_to_write + next_byte_to_read - 1 );
    } else if ( next_byte_to_read > next_byte_to_write ) { /* read pointer in front of write pointer so don't wrap around */
        return ( next_byte_to_read - next_byte_to_write - 1);
    } else { /* read and write pointers are at same place */
        return ( buffer_.size() - 1 );
    }
}

/* Write buffer to bytestreamqueue starting at next_byte_to_write */
void ByteStreamQueue::write( const string & buffer )
{
    /* current position in buffer */
    unsigned int i = 0;
    /* write until write pointer is at end of vector or you have written entire string */
    while ( next_byte_to_write < buffer_.size() && i < buffer.length() ) {
        buffer_.at( next_byte_to_write ) = buffer.data()[i];
        if ( next_byte_to_write == buffer_.size() - 1 ) {
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
    unsigned int j = 0;
    string to_be_written;

    if ( limit == 0 ) { /* nothing available to read from bytestreamqueue and write to fd*/
        return;
    }
    unsigned int current_read_ptr = next_byte_to_read;
    /* Make string to be written (of length available_to_read) but don't update read pointer yet */
    while ( current_read_ptr < buffer_.size() && j < limit ) {
        to_be_written.push_back( buffer_.at( current_read_ptr ) );
        current_read_ptr++;
        j++;
    }

    /* set current_read_ptr to beginning and continue */
    current_read_ptr = 0;
    while ( j < limit ) {
        to_be_written.push_back( buffer_.at( current_read_ptr ) );
        current_read_ptr++;
        j++;
    }

    /* Attempt to write constructed string to given file descriptor */
    auto amount_written = fd.writeamount( to_be_written );

    /* Update next_byte_to_read based on what was written */
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
