/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "delay_queue.hh"
#include "timestamp.hh"

using namespace std;

void DelayQueue::read_packet( const string & contents )
{
    packet_queue_.emplace( timestamp() + delay_ms_, contents );
}

void DelayQueue::write_packets( FileDescriptor & fd )
{
    while ( (!packet_queue_.empty())
            && (packet_queue_.front().first <= timestamp()) ) {
        fd.write( packet_queue_.front().second );
        packet_queue_.pop();
    }
}

string DelayQueue::get_next( )
{
    if ( (!packet_queue_.empty())
            && (packet_queue_.front().first <= timestamp()) ) {
        auto ret = packet_queue_.front().second;
        packet_queue_.pop();
        return ret;
    } else {
        return "";
    }
}

int DelayQueue::wait_time( void ) const
{
    return packet_queue_.empty() ? UINT16_MAX : (packet_queue_.front().first - timestamp());
}
