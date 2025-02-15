/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ABSTRACT_DUALPI2_PACKET_QUEUE_HH
#define ABSTRACT_DUALPI2_PACKET_QUEUE_HH

#include <random>
#include <thread>
#include "dropping_packet_queue.hh"

/* Max value of an 32-bit integer */
#define MAX_PROB ((uint32_t)(~((uint32_t)0)))

class AbstractDualPI2PacketQueue : public DroppingPacketQueue
{
protected:
    //This constant is copied from link_queue.hh.
    //It maybe better to get this in a more reliable way in the future.
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    uint32_t recur_count_;


public:
    AbstractDualPI2PacketQueue( const std::string & args );

    virtual void enqueue( QueuedPacket && p ) = 0;

    uint32_t get_recur_count ( void ) { return recur_count_; } 
    void set_recur_count ( uint32_t val ) { recur_count_ = val; }
};

uint32_t scale_prob( double prob );

#endif /* ABSTRACT_DUALPI2_PACKET_QUEUE_HH */
