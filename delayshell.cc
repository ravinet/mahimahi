/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#include "tundevice.hh"
#include "exception.hh"
#include "ferry.hh"
#include "child_process.hh"
#include "system_runner.hh"
#include "nat.hh"
#include "socket.hh"
#include "address.hh"
#include "util.hh"
#include "config.h"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        const uint64_t delay_ms = myatoi( argv[ 1 ] );

        /* make pair of connected sockets */
        int pipes[ 2 ];
        if ( socketpair( AF_UNIX, SOCK_DGRAM, 0, pipes ) < 0 ) {
            throw Exception( "socketpair" );
        }
        FileDescriptor egress_socket( pipes[ 0 ], "socketpair" ),
            ingress_socket( pipes[ 1 ], "socketpair" );

        /* set egress and ingress ip addresses */
        string egress_addr = "172.30.100.100";
        string ingress_addr = "172.30.100.101";

        TunDevice egress_tun( "egress", egress_addr, ingress_addr );

        /* create outside listener socket for UDP dns requests */
        Socket listener_socket_outside( SocketType::UDP );
        listener_socket_outside.bind( Address( egress_addr, "0", SocketType::UDP ) );

        /* store port outside listener socket bound to so inside socket can connect to it */
        string listener_outside_port = to_string( listener_socket_outside.local_addr().port() );

        /* address of outside dns server for UDP requests */
        Address connect_addr_outside( "localhost", "domain", SocketType::UDP );

        /* create outside listener socket for TCP dns requests */
        Socket listener_socket_outside_tcp( SocketType::TCP );
        listener_socket_outside_tcp.bind( Address( egress_addr, "0", SocketType::TCP ) );

        /* store port outside listener socket bound to so inside socket can connect to it */
        string listener_outside_port_tcp = to_string( listener_socket_outside_tcp.local_addr().port() );

        /* address of outside dns server for TCP requests */
        Address connect_addr_outside_tcp( "localhost", "domain", SocketType::TCP );

        listener_socket_outside_tcp.listen();

        /* Fork */
        ChildProcess container_process( [&]() {
                /* Unshare network namespace */
                if ( unshare( CLONE_NEWNET ) == -1 ) {
                    throw Exception( "unshare" );
                }

                TunDevice ingress_tun( "ingress", ingress_addr, egress_addr );

                /* bring up localhost */
                Socket ioctl_socket( SocketType::UDP );
                TunDevice::interface_ioctl( ioctl_socket.raw_fd(), SIOCSIFFLAGS, "lo",
                                            [] ( struct ifreq &ifr,
                                                 struct sockaddr_in & )
                                            { ifr.ifr_flags = IFF_UP; } );

                run( { ROUTE, "add", "-net", "default", "gw", egress_addr } );

                /* create inside listener socket for UDP dns requests */
                Socket listener_socket_inside( SocketType::UDP );
                listener_socket_inside.bind( Address( "localhost", "domain", SocketType::UDP ) );

                /* outside address to send UDP dns requests to */
                Address connect_addr_inside( egress_addr, listener_outside_port, SocketType::UDP );

                /* create inside listener socket for TCP dns requests */
                Socket listener_socket_inside_tcp( SocketType::TCP );
                listener_socket_inside_tcp.bind( Address( "localhost", "domain", SocketType::TCP ) );
                listener_socket_inside_tcp.listen();

                /* outside address to send TCP dns requests to */
                Address connect_addr_inside_tcp( egress_addr, listener_outside_port_tcp, SocketType::TCP );

                /* Fork again after dropping root privileges*/
                drop_privileges();

                ChildProcess bash_process( [&user_environment]() {
                        const string shell = shell_path();
                        if ( execle( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ), user_environment ) < 0 ) {
                            throw Exception( "execl" );
                        }
                        return EXIT_FAILURE;
                    } );

                return ferry( ingress_tun.fd(), egress_socket, listener_socket_inside, connect_addr_inside, listener_socket_inside_tcp, connect_addr_inside_tcp, bash_process, delay_ms );
            } );

        /* set up NAT between egress and eth0 */
        NAT nat_rule;

        return ferry( egress_tun.fd(), ingress_socket, listener_socket_outside, connect_addr_outside, listener_socket_outside_tcp, connect_addr_outside_tcp, container_process, delay_ms );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
