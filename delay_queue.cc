/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>

#include "delay_queue.hh"
#include "timestamp.hh"
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

using namespace std;

void DelayQueue::read_packet( const string & contents )
{
    string no_tun_contents = contents.substr(4);

    struct iphdr *iph = (struct iphdr *)no_tun_contents.c_str();
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;
    unsigned short iphdrlen;
    iphdrlen = iph->ihl*4;
    struct tcphdr *tcph=(struct tcphdr*)(no_tun_contents.c_str() + iphdrlen);
    if ( tcph->syn == 1 ) {
        std::map<string, float>::iterator it;
        it = delay_map.find(inet_ntoa(dest.sin_addr));
        if ( it != delay_map.end() ) {
            //cout << "delaying by: " << it->second << "for SYN: " << tcph->syn << ", DEST: " << inet_ntoa(dest.sin_addr) << ", PROT: " << iph->protocol << "PORT: " << ntohs(tcph->dest) << endl;
            if ( ntohs(tcph->dest) == 443 ) {
                usleep((it->second)*2000);
            } else {
                usleep((it->second)*1000);
            }
        }
    }
    //cout << "SYN: " << tcph->syn << ", DEST: " << inet_ntoa(dest.sin_addr) << ", PROT: " << (unsigned int) iph->protocol << "PORT: " << ntohs(tcph->dest) << endl;

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

unsigned int DelayQueue::wait_time( void ) const
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
