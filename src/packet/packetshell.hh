/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef PACKETSHELL_HH
#define PACKETSHELL_HH

#include <string>

#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "address.hh"
#include "dns_proxy.hh"
#include "event_loop.hh"
#include "socketpair.hh"

template <class FerryQueueType>
class PacketShell
{
private:
    char ** const user_environment_;
    std::pair<Address, Address> egress_ingress;
    Address nameserver_;
    TunDevice egress_tun_;
    DNSProxy dns_outside_;
    NAT nat_rule_ {};

    std::pair<UnixDomainSocket, UnixDomainSocket> pipe_;

    EventLoop event_loop_;

    const Address & egress_addr( void ) { return egress_ingress.first; }
    const Address & ingress_addr( void ) { return egress_ingress.second; }

    class Ferry : public EventLoop
    {
    public:
        int loop( FerryQueueType & ferry_queue, FileDescriptor & tun, FileDescriptor & sibling );
    };

    Address get_mahimahi_base( void ) const;

public:
    PacketShell( const std::string & device_prefix, char ** const user_environment );

    template <typename... Targs>
    void start_uplink( const std::string & shell_prefix,
                       const std::vector< std::string > & command,
                       Targs&&... Fargs );

    template <typename... Targs>
    void start_downlink( Targs&&... Fargs );

    int wait_for_exit( void );

    PacketShell( const PacketShell & other ) = delete;
    PacketShell & operator=( const PacketShell & other ) = delete;
};

#endif
