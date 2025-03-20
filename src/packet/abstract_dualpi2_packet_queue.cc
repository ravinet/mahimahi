#include <chrono>

#include "abstract_dualpi2_packet_queue.hh"
#include "dropping_packet_queue.hh"
#include "timestamp.hh"

#include <netinet/ip.h>
#include <arpa/inet.h>

#include <cstddef>

using namespace std;

void AbstractDualPI2PacketQueue::enqueue( QueuedPacket && p )
{
    std::cout << "> In enqueue. Packet of size "<< p.contents.size()  << " to enqueue: "  <<  std::endl;
    print_ipv4_header( p ) ;

    queue_size_in_bytes_ += p.contents.size();
    queue_size_in_packets_++;
    internal_queue_.emplace( std::move( p ) );
    
    std::cout << "> In enqueue. Queue size is " << size_bytes() << " bytes, or " << size_packets() 
    << " packets." <<  std::endl;
}

QueuedPacket AbstractDualPI2PacketQueue::dequeue( void )
{
    assert( not internal_queue_.empty() );

    QueuedPacket ret = std::move( internal_queue_.front() );
    internal_queue_.pop();

    queue_size_in_bytes_ -= ret.contents.size();
    queue_size_in_packets_--;

    std::cout << "> In dequeue. Queue size is " << size_bytes() << " bytes, or " << size_packets() 
    << " packets." <<  std::endl;

    return ret;
}

bool AbstractDualPI2PacketQueue::empty( void ) const
{
    return internal_queue_.empty();
}

unsigned int AbstractDualPI2PacketQueue::size_bytes( void ) const
{
    assert( queue_size_in_bytes_ >= 0 );
    return unsigned( queue_size_in_bytes_ );
}

unsigned int AbstractDualPI2PacketQueue::size_packets( void ) const
{
    assert( queue_size_in_packets_ >= 0 );
    return unsigned( queue_size_in_packets_ );
}



string AbstractDualPI2PacketQueue::to_string( void ) const
{
    // string ret = type() + " [";

    // if ( byte_limit_ ) {
    //     ret += string( "bytes=" ) + ::to_string( byte_limit_ );
    // }

    // if ( packet_limit_ ) {
    //     if ( byte_limit_ ) {
    //         ret += ", ";
    //     }

    //     ret += string( "packets=" ) + ::to_string( packet_limit_ );
    // }

    // ret += "]";

    return "";
}

QueuedPacket& AbstractDualPI2PacketQueue::peek( void ) 
{
    return internal_queue_.front();
}

uint64_t AbstractDualPI2PacketQueue::qdelay_in_ms ( uint64_t ref ) 
{
    if ( internal_queue_.empty() ) return 0;
    
    QueuedPacket& head = peek();
    return ref - head.arrival_time;
}

unsigned int get_arg( const string & args, const string & name )
{
    return DroppingPacketQueue::get_arg( args, name );
}

void print_ipv4_header( QueuedPacket & p ) 
{
    std::cout << "-- PRE IP Header Information:" << std::endl;

    std::cout << std::to_string(p.contents[0]) << std::endl;
    std::cout << std::to_string(p.contents[1]) << std::endl;
    std::cout << std::to_string(p.contents[2]) << std::endl;
    std::cout << std::to_string(p.contents[3]) << std::endl;

    
    std::cout << "-- IP Header Information:" << std::endl;
    
    struct iphdr *ip_header = (struct iphdr *) &p.contents[4];
    // Version and Header Length
    std::cout << "Version: " << (int)ip_header->version << std::endl;
    std::cout << "Header Length: " << (int)ip_header->ihl * 4 << " bytes" << std::endl;
    
    // Type of Service
    std::cout << "Type of Service: " << std::to_string(ip_header->tos) << std::endl;

    // Total Length
    std::cout << "Total Length: " << ntohs(ip_header->tot_len) << " bytes" << std::endl;

    // Identification
    std::cout << "Identification: " << ntohs(ip_header->id) << std::endl;

    // Flags and Fragment Offset
    std::cout << "Flags: " << (int)ip_header->frag_off << std::endl;

    // Time to Live
    std::cout << "TTL: " << (int)ip_header->ttl << std::endl;

    // Protocol
    std::cout << "Protocol: " << (int)ip_header->protocol << std::endl;

    // Header Checksum
    std::cout << "Checksum: " << ntohs(ip_header->check) << std::endl;

    // Source IP Address
    struct in_addr sip;
    sip.s_addr = ip_header->saddr;
    std::cout << "Source IP: " << inet_ntoa(sip) << std::endl;

    // Destination IP Address
    struct in_addr dip;
    dip.s_addr = ip_header->daddr;
    std::cout << "Destination IP: " << inet_ntoa(dip) << std::endl;
}
