/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CELL_QUEUE_HH
#define CELL_QUEUE_HH

#include <queue>
#include <cstdint>
#include <string>

#include "file_descriptor.hh"

class CellQueue
{
private:
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    struct QueuedPacket
    {
        int bytes_to_transmit;
        std::string contents;

        QueuedPacket( const std::string & s_contents );
    };

    unsigned int next_delivery_;
    std::vector< uint64_t > schedule_;
    uint64_t base_timestamp_;

    std::queue< QueuedPacket > packet_queue_;

    uint64_t next_delivery_time( void ) const;

    void use_a_delivery_opportunity( void );

public:
    CellQueue( const std::string & filename );

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    int wait_time( void ) const;
};

#endif /* CELL_QUEUE_HH */
