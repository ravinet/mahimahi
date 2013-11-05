/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BYTESTREAM_QUEUE_HH
#define BYTESTREAM_QUEUE_HH

#include <queue>
#include <string>

#include "file_descriptor.hh"

class ByteStreamQueue
{
private:
    std::vector< char > buffer_;

    unsigned int next_byte_to_read, next_byte_to_write;

public:
    ByteStreamQueue( const unsigned int size );

    /* Space available to write to bytestreamqueue */
    unsigned int available_to_write( void ) const;

    /* Space available to be read from bytestreamqueue */
    unsigned int available_to_read( void ) const;

    /* Write buffer to bytestreamqueue */
    void write( const std::string & buffer );

    /* Attempt to write available to read from bytestreamqueue to fd */
    void write_to_fd( FileDescriptor & fd );

};

#endif /* BYTESTREAM_QUEUE_HH */
