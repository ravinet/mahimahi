/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ABSTRACT_DUALPI2_PACKET_QUEUE_HH
#define ABSTRACT_DUALPI2_PACKET_QUEUE_HH

#include <queue>
// #include <cassert>

// #include <random>
// #include <thread>

#include "abstract_packet_queue.hh"

/* Max value of an 32-bit integer */
#define MAX_PROB ((uint32_t)(~((uint32_t)0)))

class AbstractDualPI2PacketQueue : public AbstractPacketQueue
{
private:
    int queue_size_in_bytes_ = 0, queue_size_in_packets_ = 0;

    std::queue<QueuedPacket> internal_queue_ {};

    virtual const std::string & type( void ) const = 0;

    uint32_t recur_count_ = 0;

protected:
    

public:
    void enqueue( QueuedPacket && p );

    QueuedPacket dequeue( void );

    bool empty( void ) const override;

    std::string to_string( void ) const override;

    unsigned int size_bytes( void ) const override;
    unsigned int size_packets( void ) const override;

    uint32_t get_recur_count ( void ) { return recur_count_; } 
    void set_recur_count ( uint32_t val ) { recur_count_ = val; }

    QueuedPacket& peek ( void );
    uint64_t qdelay_in_ns ( uint64_t ref );

        
};


// Utilities
uint32_t scale_prob( double prob );
unsigned int get_arg( const std::string & args, const std::string & name );

#endif /* ABSTRACT_DUALPI2_PACKET_QUEUE_HH */
