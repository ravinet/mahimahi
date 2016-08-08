/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DELAY_QUEUE_HH
#define DELAY_QUEUE_HH

#include <queue>
#include <cstdint>
#include <string>
#include <fstream>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include "file_descriptor.hh"

class DelayQueue
{
private:
    uint64_t delay_ms_;
    std::queue< std::pair<uint64_t, std::string> > packet_queue_;
    /* release timestamp, contents */
    std::map<std::string, float>delay_map = {};

public:
    DelayQueue( const uint64_t & s_delay_ms ) : delay_ms_( s_delay_ms ), packet_queue_() {
        std::string comp_file = "delay_ip_mapping";
        std::ifstream dfile(comp_file);
        if ( dfile ) {
            std::ifstream ifile;
            ifile.open( comp_file );
            std::string curr_line;
            while ( getline(ifile, curr_line) ) {
                // only uses first RTT specified per ip
                int space = curr_line.find(" ");
                std::string ip = curr_line.substr(0, space);
                float time = stof(curr_line.substr(space));
                if ( delay_map.count(ip) == 0 ) { // new ip
                    delay_map[ip] = time;
                }
            }
            ifile.close();
        }
    }

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void ) const;

    bool pending_output( void ) const { return wait_time() <= 0; }

    static bool finished( void ) { return false; }
};

#endif /* DELAY_QUEUE_HH */
