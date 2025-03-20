/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DUALQ_COUPLED_AQM_HH
#define DUALQ_COUPLED_AQM_HH

#include <random>
//#include <thread>
#include <chrono>
#include <atomic>
#include <netinet/ip.h>

#include "poller.hh"
#include "timerfd.hh"

#include "abstract_packet_queue.hh"
#include "l4s_packet_queue.hh"
#include "classic_packet_queue.hh"

#include "abstract_l4s_scheduler.hh"
#include "weighted_round_robin_scheduler.hh"

#define ALPHA_BETA_SHIFT 8
/* Used to scale the delay diff */
#define ALPHA_BETA_GRANULARITY 6

#define ALPHA_BETA_SCALING (ALPHA_BETA_SHIFT - ALPHA_BETA_GRANULARITY)

#define NS_PER_MS 1000000
#define NS_PER_S  1000000000

/*
   DualQ Coupled AQM, Implemented as DualQ PI2 based on RFC 9332.
*/

class DualQCoupledAQM : public AbstractPacketQueue
{
private:
    //This constant is copied from link_queue.hh.
    //It maybe better to get this in a more reliable way in the future.
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    unsigned int byte_limit_;
    unsigned int packet_limit_;

    // Proportional Integral (PI) controller parameters

    // Trigger update every...    
    uint16_t t_update_ms_;

    Poller poller_ ;
    Timerfd timer_ ;

    double alpha_;
    double beta_;

    // Coupling factor
    uint32_t k_;

    // Target queue delay
    uint64_t target_ms_;

    uint64_t l4s_qdelay_ms_;
    uint64_t classic_qdelay_ms_;

    uint32_t max_rtt_ms_;

    // Dual queues
    L4SPacketQueue l4s_queue_;
    CLASSICPacketQueue classic_queue_;

    const SchedulerType scheduler_type_;
    std::unique_ptr<AbstractL4SScheduler> scheduler_;

    uint32_t satur_drop_pkts_;

    double pp_l_; 
    double pp_;
    double p_l_;
    double p_c_;
    double p_cl_;
    double p_Cmax_;
    double p_Lmax_;

    bool l4s_drop_on_overload_;

    std::atomic<bool> update_running_ {true};


    /*
    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "dualPI2" };
        return type_;
    }
    */

    void drop( std::string reason );

    unsigned char get_ecn_bits( QueuedPacket & p );

    void mark( QueuedPacket & p );

    bool l4s_is_overloaded( void ) { return p_cl_ >= p_Lmax_; }
    bool classic_is_overloaded ( void ) { return p_c_ >= p_Cmax_; }

    void scheduler_update( void );

public:
    DualQCoupledAQM( const std::string & args );

    void enqueue( QueuedPacket && p ) override;
    QueuedPacket dequeue( void ) override;

    bool empty( void ) const override;

    std::string to_string( void ) const override;

    //static unsigned int get_arg( const std::string & args, const std::string & name );

    unsigned int size_bytes( void ) const override;
    unsigned int size_packets( void ) const override;

    bool recur( AbstractDualPI2PacketQueue & queue, double likelihood );

    void set_periodic_update( void );
    double calculate_base_aqm_prob( uint64_t ref );

    ~DualQCoupledAQM( void );
};

#endif /* DUALQ_COUPLED_AQM_HH */


