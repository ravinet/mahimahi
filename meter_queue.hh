/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef METER_QUEUE_HH
#define METER_QUEUE_HH

#include <queue>
#include <string>

#include "file_descriptor.hh"

class MeterQueue
{
private:
    std::queue<std::string> packet_queue_;
    bool graph_;

public:
    MeterQueue( const bool graph );

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void ) const;
};

#endif /* METER_QUEUE_HH */
