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

#include "config.h"
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
        std::string path_prefix = PATH_PREFIX;
        std::string comp_file = path_prefix + "/bin/delay_ip_mapping.txt";
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
                    // std::cout << "ip: " << ip << " time: " << time << std::endl;
                    delay_map[ip] = time;
                }
            }
            ifile.close();
        }
    }

    DelayQueue( const uint64_t & s_delay_ms, const std::string & ip_mapping_filename ) 
      : delay_ms_( s_delay_ms ), packet_queue_() {

        // Populate the mapping file.
        std::map<std::string, std::string> webserver_to_reverse_proxy_ip_map;
        std::ifstream ip_mapping_file(ip_mapping_filename);
        if ( ip_mapping_file ) {
          std::ifstream ip_mapping_file;
          ip_mapping_file.open( ip_mapping_filename );
          std::string curr_line;
          while ( getline(ip_mapping_file, curr_line) ) {
              // only uses first RTT specified per ip
              int space = curr_line.find(" ");
              std::string webserver_ip = curr_line.substr(0, space);
              std::string reverse_proxy_ip = curr_line.substr(space);
              size_t start = reverse_proxy_ip.find_first_not_of(' ');
              size_t end = reverse_proxy_ip.find_last_not_of(' ');
              reverse_proxy_ip = reverse_proxy_ip.substr(start, (end - start + 1));
              webserver_to_reverse_proxy_ip_map[webserver_ip] = reverse_proxy_ip;
          }
          ip_mapping_file.close();
        }

        std::string path_prefix = PATH_PREFIX;
        std::string comp_file = path_prefix + "/bin/delay_ip_mapping.txt";
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
                    if ( webserver_to_reverse_proxy_ip_map.count(ip) != 0 ) {
                      std::string reverse_proxy_ip = webserver_to_reverse_proxy_ip_map[ip];
                      delay_map[reverse_proxy_ip] = time;
                    }
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
