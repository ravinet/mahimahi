/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FILE_DELAY_QUEUE_HH
#define FILE_DELAY_QUEUE_HH

#include <queue>
#include <cstdint>
#include <string>
#include "file_descriptor.hh"

class FileDelayQueue
{
private:
    std::vector< uint64_t > delays_;
    uint64_t time_res_ms_;
    std::queue< std::pair<uint64_t, std::string> > packet_queue_;
    /* release timestamp, contents */

public:

    FileDelayQueue(const std::string & delay_file_name, uint64_t time_res_ms);

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void ) const;

    bool pending_output( void ) const { return wait_time() <= 0; }

    static bool finished( void ) { return false; }
};

#endif /* FILE_DELAY_QUEUE_HH */
