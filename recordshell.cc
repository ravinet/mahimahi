/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>

#include "nat.hh"
#include "util.hh"
#include "get_address.hh"
#include "address.hh"
#include "signalfd.hh"
#include "dns_proxy.hh"
#include "http_proxy.hh"
#include "netdevice.hh"

#include "config.h"

using namespace std;
using namespace PollerShortNames;

int eventloop( unique_ptr<DNSProxy> && dns_proxy,
               ChildProcess && child_process,
               unique_ptr<HTTPProxy> && http_proxy );

int eventloop( unique_ptr<DNSProxy> && dns_proxy,
               vector<ChildProcess> && child_processes,
               unique_ptr<HTTPProxy> && http_proxy );

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " folder_for_recorded_content" );
        }

        /* Make sure directory ends with '/' so we can prepend directory to file name for storage */
        string directory( argv[ 1 ] );
        if ( directory.back() != '/' ) {
            directory.append( "/" );
        }

        const Address nameserver = first_nameserver();

        /* set egress and ingress ip addresses */
        Interfaces interfaces;

        auto egress_octet = interfaces.first_unassigned_address( 1 );
        auto ingress_octet = interfaces.first_unassigned_address( egress_octet.second + 1 );

        Address egress_addr = egress_octet.first, ingress_addr = ingress_octet.first;

        /* make pair of devices */
        string egress_name = "veth-" + to_string( getpid() ), ingress_name = "veth-i" + to_string( getpid() );
        VirtualEthernetPair veth_devices( egress_name, ingress_name );

        /* bring up egress */
        assign_address( egress_name, egress_addr, ingress_addr );

        /* create DNS proxy */
        unique_ptr<DNSProxy> dns_outside( new DNSProxy( egress_addr, nameserver, nameserver ) );

        /* set up NAT between egress and eth0 */
        NAT nat_rule( ingress_addr );

        /* set up http proxy for tcp */
        unique_ptr<HTTPProxy> http_proxy( new HTTPProxy( egress_addr, directory ) );

        /* set up dnat */
        DNAT dnat( http_proxy->tcp_listener().local_addr(), egress_name );

        /* Fork */
        ChildProcess container_process( [&]() {
                /* bring up localhost */
                interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "lo",
                                 [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

                /* create DNS proxy if nameserver address is local */
                auto dns_inside = DNSProxy::maybe_proxy( nameserver,
                                                         dns_outside->udp_listener().local_addr(),
                                                         dns_outside->tcp_listener().local_addr() );

                /* Fork again after dropping root privileges */
                drop_privileges();

                ChildProcess bash_process( [&]() {
                        /* restore environment and tweak bash prompt */
                        environ = user_environment;
                        prepend_shell_prefix( "[record] " );

                        const string shell = shell_path();
                        SystemCall( "execl", execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) );
                        return EXIT_FAILURE;
                    } );

                return eventloop( move( dns_inside ), move( bash_process ), nullptr );
            }, true ); /* new network namespace */

        /* give ingress to container */
        run( { IP, "link", "set", "dev", ingress_name, "netns", to_string( container_process.pid() ) } );

        /* bring up ingress */
        in_network_namespace( container_process.pid(), [&] () {
                /* bring up veth device */
                assign_address( ingress_name, ingress_addr, egress_addr );

                /* create default route */
                rtentry route;
                zero( route );

                route.rt_gateway = egress_addr.raw_sockaddr();
                route.rt_dst = route.rt_genmask = Address().raw_sockaddr();
                route.rt_flags = RTF_UP | RTF_GATEWAY;

                SystemCall( "ioctl SIOCADDRT", ioctl( Socket( UDP ).fd().num(), SIOCADDRT, &route ) );
            } );

        ChildProcess recordr_process( [&]() {
                drop_privileges();

                /* check if user-specified storage folder exists, and if not, create it */
                check_storage_folder( directory );
                return eventloop( move( dns_outside ), {}, move( http_proxy ) );
            } );

        vector<ChildProcess> child_processes;
        child_processes.emplace_back( move( container_process ) );
        child_processes.emplace_back( move( recordr_process ) );

        return eventloop( nullptr, move( child_processes ), nullptr );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int eventloop( unique_ptr<DNSProxy> && dns_proxy,
               ChildProcess && child_process,
               unique_ptr<HTTPProxy> && http_proxy )
{
    vector<ChildProcess> children;
    children.emplace_back( move( child_process ) );

    return eventloop( move( dns_proxy ), move( children ), move( http_proxy ) );
}

int eventloop( unique_ptr<DNSProxy> && dns_proxy,
               vector<ChildProcess> && child_processes,
               unique_ptr<HTTPProxy> && http_proxy )
{
    /* set up signal file descriptor */
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );

    Poller poller;

    if ( dns_proxy ) {
        poller.add_action( Poller::Action( dns_proxy->udp_listener().fd(), Direction::In,
                                           [&] () {
                                               dns_proxy->handle_udp();
                                               return ResultType::Continue;
                                           } ) );

        poller.add_action( Poller::Action( dns_proxy->tcp_listener().fd(), Direction::In,
                                           [&] () {
                                               dns_proxy->handle_tcp();
                                               return ResultType::Continue;
                                           } ) );
    }

    if ( http_proxy ) {
        poller.add_action( Poller::Action( http_proxy->tcp_listener().fd(), Direction::In,
                                           [&] () {
                                               http_proxy->handle_tcp();
                                               return ResultType::Continue;
                                           } ) );
    }

    /* we get signal -> main screen turn on -> handle signal */
    poller.add_action( Poller::Action( signal_fd.fd(), Direction::In,
                                       [&] () {
                                           return handle_signal( signal_fd.read_signal(),
                                                                 child_processes );
                                       } ) );

    while ( true ) {
        auto poll_result = poller.poll( 60000 );
        if ( poll_result.result == Poller::Result::Type::Exit ) {
            return poll_result.exit_status;
        }
    }
}

