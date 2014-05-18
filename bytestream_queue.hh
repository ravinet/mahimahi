/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BYTESTREAM_QUEUE_HH
#define BYTESTREAM_QUEUE_HH

#include <queue>
#include <string>
#include <functional>
#include <memory>

#include "file_descriptor.hh"
#include "read_write_interface.hh"

class ByteStreamQueue
{
private:
    std::string buffer_;

    size_t next_byte_to_push, next_byte_to_pop;

    size_t available_to_push( void ) const;
    size_t available_to_pop( void ) const;

public:
    ByteStreamQueue( const size_t size );

    enum class Result { Success, EndOfFile };

    Result push( FileDescriptor & fd );
    void pop( FileDescriptor & fd );

    const std::function<bool(void)> space_available;
    const std::function<bool(void)> non_empty;

    size_t contiguous_space_to_push( void );
    Result push_string( const std::string & new_chunk );

    void pop_ssl( std::unique_ptr<ReadWriteInterface> && rw );

};

bool eof( const ByteStreamQueue::Result & r );

#endif /* BYTESTREAM_QUEUE_HH */
