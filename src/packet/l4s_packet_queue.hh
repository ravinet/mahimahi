/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef L4S_PACKET_QUEUE_HH
#define L4S_PACKET_QUEUE_HH

#include <random>
#include <thread>
#include "abstract_dualpi2_packet_queue.hh"

/*
   L4S queue implementation as part of the DualPI2 implentation described in RFC 9332.
*/

class L4SPacketQueue : public AbstractDualPI2PacketQueue
{
private:
    /* L4S native AQM parameters */ 
    
    uint64_t max_delay_thresh_us_;

    // Min threshold if using the range method
    uint64_t min_delay_thresh_us_;

    // Is the marking probability a step or a range function ?
    bool step_;

    // Minimum queue length for marking to apply
    uint32_t min_qlen_pkt_;

    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "l4s" };
        return type_;
    }

    

    

public:
    L4SPacketQueue( const std::string & args );

    uint32_t calculate_l4s_native_prob ( uint64_t qdelay );

};

#endif /* L4S_PACKET_QUEUE_HH */
