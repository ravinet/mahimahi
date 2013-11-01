/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BYTESTREAM_QUEUE_HH
#define BYTESTREAM_QUEUE_HH

#include <queue>
#include <string>

class ByteStreamQueue
{
private:
    std::queue< std::string > stream_queue_;

public:
    ByteStreamQueue( void ) : stream_queue_() {}

    void add( const std::string & buffer );

    bool empty( void );

    std::string head( void ) const;

    void update_head( size_t & amount_written );

};

#endif /* BYTESTREAM_QUEUE_HH */
