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

    unsigned int available_to_read( void ) const;

public:
    ByteStreamQueue( const unsigned int size );

    unsigned int available_to_write( void ) const;

    void add( const std::string & buffer );

    void write_to_fd( FileDescriptor & fd );

};

#endif /* BYTESTREAM_QUEUE_HH */
