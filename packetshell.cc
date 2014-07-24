/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <chrono>

#include <sys/socket.h>
#include <net/route.h>

#include "packetshell.hh"
#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "dns_server.hh"
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
      pipe_( UnixDomainSocket::make_pair() ),
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
                                                const vector< string > & command,
                                                Targs&&... Fargs )
{
    /* g++ bug 55914 makes this hard before version 4.9 */
    auto ferry_maker = std::bind( []( Targs&&... Fargs ) {
            return FerryQueueType( forward<Targs>( Fargs )... );
        }, forward<Targs>( Fargs )... );

    /* Fork */
    event_loop_.add_child_process( ChildProcess( "packetshell", [&]() {
            TunDevice ingress_tun( "ingress", ingress_addr(), egress_addr() );

            /* bring up localhost */
            interface_ioctl( Socket( UDP ), SIOCSIFFLAGS, "lo",
                             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

            /* create default route */
            rtentry route;
            zero( route );

            route.rt_gateway = egress_addr().raw_sockaddr();
            route.rt_dst = route.rt_genmask = Address().raw_sockaddr();
            route.rt_flags = RTF_UP | RTF_GATEWAY;

            SystemCall( "ioctl SIOCADDRT", ioctl( Socket( UDP ).num(), SIOCADDRT, &route ) );

            Ferry inner_ferry;

            /* run dnsmasq as local caching nameserver */
            inner_ferry.add_child_process( ChildProcess( start_dnsmasq( {
                        "-S", dns_outside_.udp_listener().local_addr().str( "#" ) } ) ) );

            /* Fork again after dropping root privileges */
            drop_privileges();

            inner_ferry.add_child_process( ChildProcess( join( command ), [&]() {
                    /* restore environment and tweak bash prompt */
                    environ = user_environment;
                    prepend_shell_prefix( shell_prefix );

                    return ezexec( command, true );
                } ) );

            /* allow downlink to write directly to inner namespace's TUN device */
            pipe_.first.send_fd( ingress_tun );

            FerryQueueType uplink_queue = ferry_maker();
            return inner_ferry.loop( uplink_queue, ingress_tun, egress_tun_ );
        }, true ) );  /* new network namespace */
}

template <class FerryQueueType>
template <typename... Targs>
void PacketShell<FerryQueueType>::start_downlink( Targs&&... Fargs )
{
    auto ferry_maker = std::bind( []( Targs&&... Fargs ) {
            return FerryQueueType( forward<Targs>( Fargs )... );
        }, forward<Targs>( Fargs )... );

    event_loop_.add_child_process( ChildProcess( "downlink", [&] () {
            drop_privileges();

            /* downlink packets go to inner namespace's TUN device */
            FileDescriptor ingress_tun = pipe_.second.recv_fd();

            Ferry outer_ferry;

            dns_outside_.register_handlers( outer_ferry );

            FerryQueueType downlink_queue = ferry_maker();
            return outer_ferry.loop( downlink_queue, egress_tun_, ingress_tun );
        } ) );
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

    /* ferry ready to write datagram -> send to sibling's tun device */
    add_action( Poller::Action( sibling, Direction::Out,
                                [&] () {
                                    ferry_queue.write_packets( sibling );
                                    return ResultType::Continue;
                                },
                                [&] () { return ferry_queue.wait_time() <= 0; } ) );

    return internal_loop( [&] () { return ferry_queue.wait_time(); } );
}
