/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <utility>

#include <sys/socket.h>
#include <net/route.h>
#include <signal.h>

#include "packetshell.hh"
#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "make_pipe.hh"

#include "ferry.cc"

using namespace std;
using namespace PollerShortNames;

template <class FerryType>
PacketShell<FerryType>::PacketShell( const std::string & device_prefix )
    : egress_ingress( two_unassigned_addresses() ),
      nameserver_( first_nameserver() ),
      egress_tun_( device_prefix + "-" + to_string( getpid() ) , egress_addr(), ingress_addr() ),
      nat_rule_( ingress_addr() ),
      pipe_( make_pipe() ),
      child_processes_()
{
    /* make sure environment has been cleared */
    assert( environ == nullptr );
}

template <class FerryType>
template <typename... Targs>
void PacketShell<FerryType>::start_uplink( const string & shell_prefix,
                                           char ** const user_environment,
                                           string dns_file,
                                           Targs&&... Fargs )
{
    /* g++ bug 55914 makes this hard before version 4.9 */
    auto ferry_maker = std::bind( []( Targs&&... Fargs ) { return FerryType( forward<Targs>( Fargs )... ); },
                                  forward<Targs>( Fargs )... );

    /* Fork */
    /* start dnsmasq to listen on 0.0.0.0:53 */
    child_processes_.emplace_back( [&] () {
            SystemCall( "execl", execl( DNSMASQ, "dnsmasq", "-k", "--no-hosts",
                                        "-H", dns_file.c_str(),
                                        static_cast<char *>( nullptr ) ) );
            return EXIT_FAILURE;
    }, false, SIGTERM );
    child_processes_.emplace_back( [&]() {
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

            /* Fork again after dropping root privileges */
            drop_privileges();

            ChildProcess bash_process( [&]() {
                    /* restore environment and tweak bash prompt */
                    environ = user_environment;
                    prepend_shell_prefix( shell_prefix );

                    const string shell = shell_path();
                    SystemCall( "execl", execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) );
                    return EXIT_FAILURE;
                } );

            FerryType uplink_queue = ferry_maker();
            return packet_ferry( uplink_queue, ingress_tun.fd(), pipe_.first,
                                 {}, move( bash_process ) );
        }, true );  /* new network namespace */
}
template <class FerryType>
template <typename... Targs>
void PacketShell<FerryType>::start_downlink( Targs&&... Fargs )
{
    auto ferry_maker = std::bind( []( Targs&&... Fargs ) { return FerryType( forward<Targs>( Fargs )... ); },
                                  forward<Targs>( Fargs )... );

    child_processes_.emplace_back( [&] () {
            drop_privileges();

            FerryType downlink_queue = ferry_maker();
            return packet_ferry( downlink_queue, egress_tun_.fd(),
                                 pipe_.second, {}, {} );
        } );
}

template <class FerryType>
int PacketShell<FerryType>::wait_for_exit( void )
{
    /* wait for either child to finish */
    Poller poller;
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );

    poller.add_action( Poller::Action( signal_fd.fd(), Direction::In,
                                       [&] () {
                                           return handle_signal( signal_fd.read_signal(),
                                                                 child_processes_ );
                                       } ) );

    while ( true ) {
        auto poll_result = poller.poll( -1 );

        if ( poll_result.result == Poller::Result::Type::Exit ) {
            return poll_result.exit_status;
        }
    }
}
