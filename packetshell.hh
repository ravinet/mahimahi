/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef PACKETSHELL_HH
#define PACKETSHELL_HH

#include <memory>

#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "get_address.hh"
#include "address.hh"
#include "dns_proxy.hh"

class PacketShell
{
private:
    std::vector<std::pair<Address, Address>> egress_ingress;
    Address nameserver_;
    TunDevice egress_cell_tun_;
    TunDevice egress_wifi_tun_;
    std::unique_ptr<DNSProxy> dns_outside_;
    NAT nat_cell_;
    NAT nat_wifi_;

    std::pair<FileDescriptor, FileDescriptor> cell_pipe_;
    std::pair<FileDescriptor, FileDescriptor> wifi_pipe_;
    std::vector<ChildProcess> child_processes_;

public:
    PacketShell( const std::string & device_prefix );

    void start_uplink( const std::string & shell_prefix,
                       char ** const user_environment,
                       const uint64_t cell_delay,
                       const uint64_t wifi_delay,
                       const std::string & cell_uplink,
                       const std::string & wifi_uplink,
                       const std::vector< std::string > & command );

    void start_downlink( const uint64_t cell_delay,
                         const uint64_t wifi_delay,
                         const std::string & cell_downlink,
                         const std::string & wifi_downlink );

    int wait_on_processes( std::vector<ChildProcess> && process_vector );
    int wait_for_exit() { return wait_on_processes( std::move( child_processes_ ) ); }

    PacketShell( const PacketShell & other ) = delete;
    PacketShell & operator=( const PacketShell & other ) = delete;
};

#endif
