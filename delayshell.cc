/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>

#include "netdevice.hh"
#include "ferry.hh"
#include "nat.hh"
#include "util.hh"
#include "get_address.hh"
#include "dnat.hh"
#include "address.hh"

#include "config.h"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " propagation-delay [in milliseconds]" );
        }

        const uint64_t delay_ms = myatoi( argv[ 1 ] );
        const Address nameserver = first_nameserver();

        /* make pair of connected sockets */
        int pipes[ 2 ];
        SystemCall( "socketpair", socketpair( AF_UNIX, SOCK_DGRAM, 0, pipes ) );
        FileDescriptor egress_socket( pipes[ 0 ] ), ingress_socket( pipes[ 1 ] );

        /* set egress and ingress ip addresses */
        Interfaces interfaces;

        auto egress_octet = interfaces.first_unassigned_address( 1 );
        auto ingress_octet = interfaces.first_unassigned_address( egress_octet.second + 1 );

        Address egress_addr = egress_octet.first, ingress_addr = ingress_octet.first;

        TunDevice egress_tun( "delayshell" + to_string( getpid() ) , egress_addr.ip(), ingress_addr.ip() );

        /* create DNS proxy */
        unique_ptr<DNSProxy> dns_outside( new DNSProxy( egress_addr, nameserver, nameserver ) );

        /* set up NAT between egress and eth0 */
        NAT nat_rule;

        /* Fork */
        ChildProcess container_process( [&]() {
                TunDevice ingress_tun( "ingress", ingress_addr.ip(), egress_addr.ip() );

                /* bring up localhost */
                Socket ioctl_socket( UDP );
                interface_ioctl( ioctl_socket.fd(), SIOCSIFFLAGS, "lo",
                                 [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

                /* create default route */
                struct rtentry route;
                zero( route );

                route.rt_gateway = egress_addr.raw_sockaddr();
                route.rt_dst = route.rt_genmask = Address().raw_sockaddr();
                route.rt_flags = RTF_UP | RTF_GATEWAY;

                SystemCall( "ioctl SIOCADDRT", ioctl( ioctl_socket.fd().num(), SIOCADDRT, &route ) );

                /* create DNS proxy if nameserver address is local */
                auto dns_inside = DNSProxy::maybe_proxy( nameserver,
                                                         dns_outside->udp_listener().local_addr(),
                                                         dns_outside->tcp_listener().local_addr() );

                /* Fork again after dropping root privileges */
                drop_privileges();

                ChildProcess bash_process( [&]() {
                        /* restore environment and tweak bash prompt */
                        environ = user_environment;
                        prepend_shell_prefix( "[delay " + to_string( delay_ms ) + "] " );

                        const string shell = shell_path();
                        SystemCall( "execl", execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) );
                        return EXIT_FAILURE;
                    } );

                return ferry_with_delay( ingress_tun.fd(), egress_socket, move( dns_inside ), bash_process, delay_ms );
            }, true );  /* new network namespace */

        return ferry_with_delay( egress_tun.fd(), ingress_socket, move( dns_outside ), container_process, delay_ms );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
