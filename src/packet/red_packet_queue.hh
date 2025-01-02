/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef RED_PACKET_QUEUE_HH
#define RED_PACKET_QUEUE_HH

#include <string>
#include "util.hh"
#include <iostream>

#include <fstream>
#include <memory>
#include <deque>
#include <random>
#include "dropping_packet_queue.hh"

/*
   Random Early Detection (RED) AQM Implementation.
   See section 3 of https://tools.ietf.org/html/rfc2309#ref-Jacobson88
   for a description of the queuing discipline.
   j

*/
class REDPacketQueue : public DroppingPacketQueue
{
private:
    //Configuration parameters
    double wq_, min_thresh_, max_thresh_;
    uint32_t transmission_time_;
    uint64_t time_at_zero_q_;
    std::default_random_engine prng_;
    std::uniform_real_distribution<float> drop_dist_;
    float current_random_val_;
    uint32_t count_;
      
    const std::string & type( void ) const override
    {
        static const std::string type_ { "red" };
        return type_;
    }

    double weighted_average_ = 0;
    unsigned int max_queue_depth_packets() const;

public:
    REDPacketQueue( ParsedArguments & args );
    QueuedPacket dequeue( void ) override;
    void enqueue( QueuedPacket && p ) override;
};

#endif