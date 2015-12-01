/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef TRIVIAL_QUEUE_HH
#define TRIVIAL_QUEUE_HH

#include <queue>
#include <cstdint>
#include <string>
#include <limits>

#include "file_descriptor.hh"

class TrivialQueue
{
private:
    std::queue<std::string> packet_queue_ {};

public:
    TrivialQueue( int ) {}

    void read_packet( const std::string & contents )
    {
        packet_queue_.emplace( contents );
    }

    void write_packets( FileDescriptor & fd )
    {
        while ( not packet_queue_.empty() ) {
            fd.write( packet_queue_.front() );
            packet_queue_.pop();
        }
    }

    unsigned int wait_time( void ) const
    {
        return packet_queue_.empty() ? std::numeric_limits<uint16_t>::max() : 0;
    }

    bool pending_output( void ) const { return not packet_queue_.empty(); }

    static bool finished( void ) { return false; }
};

#endif /* TRIVIAL_QUEUE_HH */
