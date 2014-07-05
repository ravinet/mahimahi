/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <utility>

#include <sys/socket.h>
#include <net/route.h>

#include "packetshell.hh"
#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "make_pipe.hh"
#include "config.h"

using namespace std;
using namespace PollerShortNames;

template <class FerryQueueType>
PacketShell<FerryQueueType>::PacketShell( const std::string & device_prefix )
    : egress_ingress( two_unassigned_addresses() ),
      nameserver_( first_nameserver() ),
      egress_tun_( device_prefix + "-" + to_string( getpid() ) , egress_addr(), ingress_addr() ),
      dns_outside_( Address( egress_addr().ip(), "domain" ), nameserver_, nameserver_ ),
      nat_rule_( ingress_addr() ),
      pipe_( make_pipe() ),
      event_loop_()
{
    /* make sure environment has been cleared */
    if ( environ != nullptr ) {
        throw Exception( "PacketShell", "environment was not cleared" );
    }
}

template <class FerryQueueType>
template <typename... Targs>
void PacketShell<FerryQueueType>::start_uplink( const string & shell_prefix,
                                                char ** const user_environment,
                                                Targs&&... Fargs )
{
    /* g++ bug 55914 makes this hard before version 4.9 */
    auto ferry_maker = std::bind( []( Targs&&... Fargs ) {
            return FerryQueueType( forward<Targs>( Fargs )... );
        }, forward<Targs>( Fargs )... );

    /* Fork */
    event_loop_.add_child_process( [&]() {
            TunDevice ingress_tun( "ingress", ingress_addr(), egress_addr() );

            /* bring up localhost */
            Socket ioctl_socket( UDP );
            interface_ioctl( ioctl_socket.fd(), SIOCSIFFLAGS, "lo",
                             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

            /* create default route */
            rtentry route;
            zero( route );

            route.rt_gateway = egress_addr().raw_sockaddr();
            route.rt_dst = route.rt_genmask = Address().raw_sockaddr();
            route.rt_flags = RTF_UP | RTF_GATEWAY;

            SystemCall( "ioctl SIOCADDRT", ioctl( ioctl_socket.fd().num(), SIOCADDRT, &route ) );

            Ferry inner_ferry;

            /* run dnsmasq as local caching nameserver */
            inner_ferry.add_child_process( [&]() {
                    SystemCall( "execl", execl( DNSMASQ, "dnsmasq", "--keep-in-foreground",
                                                "--no-resolv", "-S",
                                                dns_outside_.udp_listener().local_addr().str( "#" ).c_str(),
                                                "-C", "/dev/null", "-u", username().c_str(), static_cast<char *>( nullptr ) ) );
                    return EXIT_FAILURE;
                }, false, SIGTERM );

            /* Fork again after dropping root privileges */
            drop_privileges();

            inner_ferry.add_child_process( [&]() {
                    /* restore environment and tweak bash prompt */
                    environ = user_environment;
                    prepend_shell_prefix( shell_prefix );

                    const string shell = shell_path();
                    SystemCall( "execl", execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) );
                    return EXIT_FAILURE;
                } );

            FerryQueueType uplink_queue = ferry_maker();
            return inner_ferry.loop( uplink_queue, ingress_tun.fd(), pipe_.first );
        }, true );  /* new network namespace */
}

template <class FerryQueueType>
template <typename... Targs>
void PacketShell<FerryQueueType>::start_downlink( Targs&&... Fargs )
{
    auto ferry_maker = std::bind( []( Targs&&... Fargs ) {
            return FerryQueueType( forward<Targs>( Fargs )... );
        }, forward<Targs>( Fargs )... );

    event_loop_.add_child_process( [&] () {
            drop_privileges();

            Ferry outer_ferry;

            dns_outside_.register_handlers( outer_ferry );

            FerryQueueType downlink_queue = ferry_maker();
            return outer_ferry.loop( downlink_queue, egress_tun_.fd(), pipe_.second );
        } );
}

template <class FerryQueueType>
int PacketShell<FerryQueueType>::wait_for_exit( void )
{
    return event_loop_.loop();
}

template <class FerryQueueType>
int PacketShell<FerryQueueType>::Ferry::loop( FerryQueueType & ferry_queue,
                                              FileDescriptor & tun,
                                              FileDescriptor & sibling )
{
    /* tun device gets datagram -> read it -> give to ferry */
    add_simple_input_handler( tun, 
                              [&] () {
                                  ferry_queue.read_packet( tun.read() );
                                  return ResultType::Continue;
                              } );

    /* we get datagram from sibling process -> write it to tun device */
    add_simple_input_handler( sibling,
                              [&] () {
                                  tun.write( sibling.read() );
                                  return ResultType::Continue;
                              } );

    /* ferry ready to write datagram -> send to sibling process */
    add_action( Poller::Action( sibling, Direction::Out,
                                [&] () {
                                    ferry_queue.write_packets( sibling );
                                    return ResultType::Continue;
                                },
                                [&] () { return ferry_queue.wait_time() <= 0; } ) );

    return internal_loop( [&] () { return ferry_queue.wait_time(); } );
}
