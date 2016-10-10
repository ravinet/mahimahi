/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CODEL_PACKET_QUEUE_HH
#define CODEL_PACKET_QUEUE_HH

#include <random>
#include "dropping_packet_queue.hh"

struct dodequeue_result {
    QueuedPacket p;
    bool ok_to_drop;

    dodequeue_result ( )
        : p ( "", 0 ), ok_to_drop ( false )
    {}
};

/*
   Controlled Delay (CoDel) AQM Implementation
   based on IETF draft-ietf-aqm-codel-04
   and the CoDel implementation in Linux 4.4
   Contributed by
      Joseph D. Beshay <joseph.beshay@utdallas.edu>
*/
class CODELPacketQueue : public DroppingPacketQueue
{
private:
    const static unsigned int PACKET_SIZE = 1504;
    //Configuration parameters
    uint32_t target_, interval_;

    //State variables
    uint64_t first_above_time_, drop_next_;
    uint32_t count_, lastcount_;
    bool dropping_;


    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "codel" };
        return type_;
    }

    dodequeue_result dodequeue ( uint64_t now );
    uint64_t control_law ( uint64_t t, uint32_t count );

public:
    CODELPacketQueue( const std::string & args );

    void enqueue( QueuedPacket && p ) override;

    QueuedPacket dequeue( void ) override;
};

#endif /* PIE_PACKET_QUEUE_HH */ 
