/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef PACKETSHELL_HH
#define PACKETSHELL_HH

#include <memory>

#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "dns_proxy.hh"

template <class FerryType>
class PacketShell
{
private:
    std::pair<Address, Address> egress_ingress;
    Address nameserver_;
    TunDevice egress_tun_;
    std::unique_ptr<DNSProxy> dns_outside_;
    NAT nat_rule_ {};

    std::pair<FileDescriptor, FileDescriptor> pipe_;

    std::vector<ChildProcess> child_processes_;

    SignalMask eventloop_signals_;

    const Address & egress_addr( void ) { return egress_ingress.first; }
    const Address & ingress_addr( void ) { return egress_ingress.second; }

public:
    PacketShell( const std::string & device_prefix );

    template <typename... Targs>
    void start_uplink( const std::string & shell_prefix,
                       char ** const user_environment,
                       Targs&&... Fargs );

    template <typename... Targs>
    void start_downlink( Targs&&... Fargs );

    int wait_for_exit( void );

    PacketShell( const PacketShell & other ) = delete;
    PacketShell & operator=( const PacketShell & other ) = delete;
};

#endif
