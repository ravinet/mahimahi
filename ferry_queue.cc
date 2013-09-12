/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "ferry_queue.hh"
#include "timestamp.hh"

using namespace std;

void FerryQueue::read_packet( const std::string & contents )
{
    packet_queue_.emplace( timestamp() + delay_ms_, contents );
}

void FerryQueue::write_packets( TapDevice & output )
{
    while ( (!packet_queue_.empty())
            && (packet_queue_.front().first <= timestamp()) ) {
        output.write( packet_queue_.front().second );
        packet_queue_.pop();
    }
}

int FerryQueue::wait_time( void ) const
{
    return packet_queue_.empty() ? UINT16_MAX : (packet_queue_.front().first - timestamp());
}
