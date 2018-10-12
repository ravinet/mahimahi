/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef LINK_QUEUE_HH
#define LINK_QUEUE_HH

#include <queue>
#include <cstdint>
#include <string>
#include <fstream>
#include <memory>

#include "file_descriptor.hh"
#include "binned_livegraph.hh"
#include "abstract_packet_queue.hh"

class LinkQueue
{
private:
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    unsigned int next_delivery_;
    std::vector<uint64_t> schedule_;
    uint64_t base_timestamp_;

    std::unique_ptr<AbstractPacketQueue> packet_queue_;
    QueuedPacket packet_in_transit_;
    unsigned int packet_in_transit_bytes_left_;
    std::queue<std::string> output_queue_;

    std::unique_ptr<std::ofstream> log_;
    std::unique_ptr<BinnedLiveGraph> throughput_graph_;
    std::unique_ptr<BinnedLiveGraph> delay_graph_;

    bool repeat_;
    bool finished_;

    uint64_t next_delivery_time( void ) const;

    void use_a_delivery_opportunity( void );

    void record_arrival( const uint64_t arrival_time, const size_t pkt_size );
    void record_drop( const uint64_t time, const size_t pkts_dropped, const size_t bytes_dropped );
    void record_departure_opportunity( void );
    void record_departure( const uint64_t departure_time, const QueuedPacket & packet );

    void rationalize( const uint64_t now );
    void dequeue_packet( void );

public:
    LinkQueue( const std::string & link_name, const std::string & filename, const std::string & logfile,
               const bool repeat, const bool graph_throughput, const bool graph_delay,
               std::unique_ptr<AbstractPacketQueue> && packet_queue,
               const std::string & command_line );

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void );

    bool pending_output( void ) const;

    bool finished( void ) const { return finished_; }
};

#endif /* LINK_QUEUE_HH */
