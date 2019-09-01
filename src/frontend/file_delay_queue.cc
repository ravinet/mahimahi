/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>

#include "file_delay_queue.hh"
#include "timestamp.hh"

using namespace std;

void FileDelayQueue::read_packet( const string & contents )
{
    uint64_t delay_index = int(timestamp()/(double)time_resolution + 0.5);
    uint64_t delay = delays[delay_index];
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
