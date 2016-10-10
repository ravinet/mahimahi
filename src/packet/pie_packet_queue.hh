/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef PIE_PACKET_QUEUE_HH
#define PIE_PACKET_QUEUE_HH

#include <random>
#include <thread>
#include "dropping_packet_queue.hh"

/*    
   Proportional Integral controller Enhanced (PIE)
   AQM Implementation based on draft-ietf-aqm-pie-09
   and the PIE implementation in Linux 4.4
   Contributed by
      Joseph D. Beshay <joseph.beshay@utdallas.edu>
*/
class PIEPacketQueue : public DroppingPacketQueue
{
private:
    //This constant is copied from link_queue.hh.
    //It maybe better to get this in a more reliable way in the future.
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    //Configurable parameters
    uint32_t qdelay_ref_, max_burst_;

    //Internal parameters
    double alpha_, beta_;
    uint32_t t_update_;     // ms
    uint32_t dq_threshold_; // bytes

    //Status variables
    double drop_prob_;
    uint32_t burst_allowance_, qdelay_old_, current_qdelay_;
    uint32_t dq_count_, dq_tstamp_, avg_dq_rate_;

    //Implementation specific
    std::uniform_real_distribution<double> uniform_generator_;
    std::default_random_engine prng_;
    uint64_t last_update_;
    


    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "pie" };
        return type_;
    }

    bool drop_early ( void );

    void calculate_drop_prob ( void );

public:
    PIEPacketQueue( const std::string & args );

    void enqueue( QueuedPacket && p ) override;

    QueuedPacket dequeue( void ) override;
};

#endif /* PIE_PACKET_QUEUE_HH */ 
