/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef METER_QUEUE_HH
#define METER_QUEUE_HH

#include <queue>
#include <string>
#include <memory>

#include "file_descriptor.hh"
#include "graph.hh"

class MeterQueue
{
private:
    std::queue<std::string> packet_queue_;
    std::unique_ptr<Graph> graph_;

    unsigned int bytes_this_bin_;
    unsigned int bin_width_;
    uint64_t current_bin_;
    double logical_width_;
    double logical_height_;

    uint64_t advance( void );

public:
    MeterQueue( const std::string & name, const bool graph );

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void ) const;
};

#endif /* METER_QUEUE_HH */
