/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef METER_QUEUE_HH
#define METER_QUEUE_HH

#include <queue>
#include <string>
#include <memory>

#include "file_descriptor.hh"
#include "binned_livegraph.hh"

class MeterQueue
{
private:
    std::queue<std::string> packet_queue_;
    std::unique_ptr<BinnedLiveGraph> graph_;

public:
    MeterQueue( const std::string & name, const bool graph );

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void ) const;

    bool pending_output( void ) const { return not packet_queue_.empty(); }

    static bool finished( void ) { return false; }
};

#endif /* METER_QUEUE_HH */
