/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FILE_DELAY_QUEUE_HH
#define FILE_DELAY_QUEUE_HH

#include <queue>
#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>

#include "file_descriptor.hh"

class FileDelayQueue
{
private:
    std::vector< uint64_t > delays;
    long double time_resolution;
    std::queue< std::pair<uint64_t, std::string> > packet_queue_;
    /* release timestamp, contents */

public:
    FileDelayQueue(std::string delay_file_name, long double time_res_ms) {

        std::ifstream delay_file(delay_file_name);
        std::string line;
        uint64_t delay;

        /* Read file, line-by-line. */
        while (std::getline(delay_file, line))
        {
            delay = std::stoi(line);
            delays.push_back(delay);
            std::cout << "Delay: " << delay << " ms. \n";
        }
        
        /* Set time resolution of delays in file. */
        time_resolution = time_res_ms;
    }

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void ) const;

    bool pending_output( void ) const { return wait_time() <= 0; }

    static bool finished( void ) { return false; }
};

#endif /* FILE_DELAY_QUEUE_HH */
