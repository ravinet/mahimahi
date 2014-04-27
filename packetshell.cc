/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <utility>

#include <sys/socket.h>
#include <net/route.h>
#include <signal.h>

#include "packetshell.hh"
#include "rate_delay_queue.hh"
#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "get_address.hh"
#include "address.hh"
#include "make_pipe.hh"

#include "ferry.cc"

using namespace std;
using namespace PollerShortNames;

PacketShell::PacketShell( const std::string & device_prefix )
    : egress_ingress( get_unassigned_address_pairs(2) ),
      nameserver_( first_nameserver() ),
      egress_cell_tun_( device_prefix + "-cell-" + to_string( getpid() ) , egress_ingress.at(0).first, egress_ingress.at(0).second ),
      egress_wifi_tun_( device_prefix + "-wifi-" + to_string( getpid() ) , egress_ingress.at(1).first, egress_ingress.at(1).second ),
      dns_outside_( new DNSProxy( egress_ingress.at(0).first, nameserver_, nameserver_ ) ),
      nat_cell_( egress_ingress.at(0).second ),
      nat_wifi_( egress_ingress.at(1).second ),
      cell_pipe_( make_pipe() ),
      wifi_pipe_( make_pipe() ),
      child_processes_()
{
    /* make sure environment has been cleared */
    assert( environ == nullptr );

    /* constructor will throw exception if it fails, so we should not be able to get nullptr */
    assert( dns_outside_ );
}

void PacketShell::start_uplink( const std::string & shell_prefix,
                                char ** const user_environment,
                                const uint64_t cell_delay,
                                const uint64_t wifi_delay,
                                const std::string & cell_uplink,
                                const std::string & wifi_uplink,
                                const vector< string > & command )
{
    /* Fork */
    child_processes_.emplace_back( [&]() {
            TunDevice ingress_cell_tun( "ingress-cell", egress_ingress.at(0).second, egress_ingress.at(0).first );
            TunDevice ingress_wifi_tun( "ingress-wifi", egress_ingress.at(1).second, egress_ingress.at(1).first );
            /* bring up localhost */
            Socket ioctl_socket( UDP );
            interface_ioctl( ioctl_socket.fd(), SIOCSIFFLAGS, "lo",
                             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

            /* create default route through cell and wifi */
            rtentry route;
            zero( route );

            route.rt_gateway = egress_ingress.at(0).first.raw_sockaddr();
            route.rt_dst = route.rt_genmask = Address().raw_sockaddr();
            route.rt_flags = RTF_UP | RTF_GATEWAY;

            SystemCall( "ioctl SIOCADDRT", ioctl( ioctl_socket.fd().num(), SIOCADDRT, &route ) );

            /* Policy routing to simulate two default interfaces */
            run( {"/sbin/ip", "route", "add", egress_ingress.at(1).first.ip(), "dev", "ingress-wifi", "src", egress_ingress.at(1).second.ip(), "table", "1"} );
            run( {"/sbin/ip", "route", "add", "default", "via", egress_ingress.at(1).first.ip(), "dev", "ingress-wifi","table", "1"} );
            run( {"/sbin/ip", "rule", "add", "from", egress_ingress.at(1).second.ip(),"table", "1"} );
            run( {"/sbin/ip", "rule", "add", "to", egress_ingress.at(1).second.ip(), "table", "1"} );
            run( {"/sbin/ip", "route", "flush", "cache"} );

            /* create DNS proxy if nameserver address is local */
            auto dns_inside = DNSProxy::maybe_proxy( nameserver_,
                                                     dns_outside_->udp_listener().local_addr(),
                                                     dns_outside_->tcp_listener().local_addr() );

            /* Fork again after dropping root privileges */
            drop_privileges();

            ChildProcess bash_process( [&]() {
                    /* restore environment and tweak bash prompt */
                    environ = user_environment;
                    prepend_shell_prefix( shell_prefix );

                    const string shell = shell_path();
                    run( command, user_environment );
                    //SystemCall( "execl", execl( shell.c_str(), shell.c_str(), "-c", program_name.c_str(), static_cast<const char*>( nullptr ) ) );
                    return EXIT_FAILURE;
                } );

            ChildProcess cell_ferry( [&]() {
                    RateDelayQueue cell_queue( cell_delay, cell_uplink );
                    return packet_ferry( cell_queue, ingress_cell_tun.fd(), cell_pipe_.first,
                                         move( dns_inside ),
                                         {} );
                } );

            ChildProcess wifi_ferry( [&]() {
                    RateDelayQueue wifi_queue( wifi_delay, wifi_uplink);
                    return packet_ferry( wifi_queue, ingress_wifi_tun.fd(), wifi_pipe_.first,
                                         nullptr,
                                         {} );
                } );
            std::vector<ChildProcess> uplink_processes;
            uplink_processes.emplace_back( move( bash_process ) );
            uplink_processes.emplace_back( move( cell_ferry ) );
            uplink_processes.emplace_back( move( wifi_ferry ) );
            return wait_on_processes( std::move( uplink_processes ) );
        }, true );  /* new network namespace */
}

void PacketShell::start_downlink( const uint64_t cell_delay,
                                  const uint64_t wifi_delay,
                                  const std::string & cell_downlink,
                                  const std::string & wifi_downlink )
{
    child_processes_.emplace_back( [&] () {
            drop_privileges();

            RateDelayQueue cell_queue( cell_delay, cell_downlink );
            return packet_ferry( cell_queue, egress_cell_tun_.fd(),
                                 cell_pipe_.second, move( dns_outside_ ), {} );
        } );

    child_processes_.emplace_back( [&] () {
            drop_privileges();

            RateDelayQueue wifi_queue( wifi_delay, wifi_downlink );
            return packet_ferry( wifi_queue, egress_wifi_tun_.fd(),
                                 wifi_pipe_.second, nullptr, {} );
        } );
}

int PacketShell::wait_on_processes( std::vector<ChildProcess> && process_vector )
{
    /* wait for either child to finish */
    Poller poller;
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );

    poller.add_action( Poller::Action( signal_fd.fd(), Direction::In,
                                       [&] () {
                                           return handle_signal( signal_fd.read_signal(),
                                                                 process_vector );
                                       } ) );

    while ( true ) {
        auto poll_result = poller.poll( -1 );

        if ( poll_result.result == Poller::Result::Type::Exit ) {
            return poll_result.exit_status;
        }
    }
}
