/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef LOSS_QUEUE_HH
#define LOSS_QUEUE_HH

#include <queue>
#include <cstdint>
#include <string>

#include "file_descriptor.hh"

class LossQueue
{
private:
    uint64_t loss_percentage_;
    std::queue< std::pair<bool, std::string> > packet_queue_;
    /* release timestamp, contents */

public:
    LossQueue( const uint64_t & s_loss_percentage ) : loss_percentage_( s_loss_percentage ), packet_queue_() {}

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void ) const;
};

#endif /* LOSS_QUEUE_HH */
