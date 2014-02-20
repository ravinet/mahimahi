/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef RATE_DELAY_QUEUE_HH
#define RATE_DELAY_QUEUE_HH

#include <string>

#include "cell_queue.hh"
#include "delay_queue.hh"

class RateDelayQueue
{
/* Delay Queue followed by cell queue in series */
private:
    DelayQueue delay_queue_;
    CellQueue cell_queue_;

public:
    RateDelayQueue( const uint64_t & s_delay_ms, const std::string & filename ) : delay_queue_( s_delay_ms ), cell_queue_( filename ) {}

    void read_packet( const std::string & contents ) { delay_queue_.read_packet( contents ); }

    void write_packets( FileDescriptor & fd );

    int wait_time( void ) const { return std::min(delay_queue_.wait_time(), cell_queue_.wait_time()); }
};

#endif /* RATE_DELAY_QUEUE_HH */
