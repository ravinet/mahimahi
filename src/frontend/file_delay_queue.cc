/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>
#include <fstream>
#include <iostream>
#include "file_delay_queue.hh"
#include "timestamp.hh"

using namespace std;

FileDelayQueue::FileDelayQueue(const std::string & delay_file_name, uint64_t time_res_ms) : delays_(), time_res_ms_(time_res_ms), packet_queue_() 
{
    ifstream delay_file(delay_file_name);
    uint64_t delay;

    if (not delay_file.good()) {
        throw runtime_error(delay_file_name + ": error while opening for reading.");
    }

    /* Read file, line-by-line. */
    string line;
    while (delay_file.good() and getline(delay_file, line))
    {
        delay = stoi(line);
        delays_.push_back(delay);
    }    
}

void FileDelayQueue::read_packet( const string & contents )
{
    uint64_t delay_index = timestamp()/time_res_ms_;
    uint64_t delay = delays_[delay_index % delays_.size()];
    packet_queue_.emplace(timestamp() + delay, contents );
}

void FileDelayQueue::write_packets( FileDescriptor & fd )
{
    while ( (!packet_queue_.empty())
            && (packet_queue_.front().first <= timestamp()) ) {
        fd.write( packet_queue_.front().second );
        packet_queue_.pop();
    }
}

unsigned int FileDelayQueue::wait_time( void ) const
{
    if ( packet_queue_.empty() ) {
        return numeric_limits<uint16_t>::max();
    }

    const auto now = timestamp();

    if ( packet_queue_.front().first <= now ) {
        return 0;
    } else {
        return packet_queue_.front().first - now;
    }
}
