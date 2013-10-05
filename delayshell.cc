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
#include "dns_proxy.hh"

#include "config.h"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        const string delay = argv[ 1 ];
        const uint64_t delay_ms = myatoi( delay );
        const Address nameserver = first_nameserver();

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

        /* create DNS proxy */
        DNSProxy dns_outside( Address( egress_addr, 0 ), nameserver, nameserver );

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

                /* create DNS proxy */
                DNSProxy dns_inside( nameserver, dns_outside.udp_listener().local_addr(), dns_outside.tcp_listener().local_addr() );

                /* Fork again after dropping root privileges */
                drop_privileges();

                ChildProcess bash_process( [&]() {
                        /* restore environment and tweak bash prompt */
                        environ = user_environment;
                        if ( setenv( "MAHIMAHI_DELAY", delay.c_str(), true ) < 0 ) {
                            throw Exception( "setenv" );
                        }

                        if ( setenv( "PROMPT_COMMAND", "PS1=\"[delay $MAHIMAHI_DELAY] $PS1\" PROMPT_COMMAND=", false ) < 0 ) {
                            throw Exception( "setenv" );
                        }

                        const string shell = shell_path();
                        if ( execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) < 0 ) {
                            throw Exception( "execl" );
                        }
                        return EXIT_FAILURE;
                    } );

                return ferry( ingress_tun.fd(), egress_socket, dns_inside, bash_process, delay_ms );
            } );

        /* set up NAT between egress and eth0 */
        NAT nat_rule;

        return ferry( egress_tun.fd(), ingress_socket, dns_outside, container_process, delay_ms );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
