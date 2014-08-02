/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>
#include <functional>

#include "nat.hh"
#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "dns_proxy.hh"
#include "http_proxy.hh"
#include "netdevice.hh"
#include "event_loop.hh"
#include "socketpair.hh"
#include "process_recorder.hh"
#include "config.h"
#include "local_proxy.hh"

using namespace std;

template <class StoreType>
int ProcessRecorder<StoreType>::record_process( std::function<int( FileDescriptor & )> && child_procedure,
                                                Socket && socket_output,
                                                const int & veth_counter,
                                                bool record,
                                                const string & stdin_input )
{
    TemporarilyRoot tr;

    const Address nameserver = first_nameserver();

    /* set egress and ingress ip addresses */
    Interfaces interfaces;

    auto egress_octet = interfaces.first_unassigned_address( 1 );
    auto ingress_octet = interfaces.first_unassigned_address( egress_octet.second + 1 );

    Address egress_addr = egress_octet.first, ingress_addr = ingress_octet.first;

    /* make pair of devices */
    const string unique_id = to_string( getpid() ) + "-" + to_string( veth_counter );
    string egress_name = "veth-" + unique_id, ingress_name = "veth-i" + unique_id;

    VirtualEthernetPair veth_devices( egress_name, ingress_name );

    /* bring up egress */
    assign_address( egress_name, egress_addr, ingress_addr );

    Address new_egress_addr( egress_addr.ip(), 5555 );

    /* create DNS proxy */
    DNSProxy dns_outside( egress_addr, nameserver, nameserver );

    /* set up NAT between egress and eth0 */
    NAT nat_rule( ingress_addr );

    /* set up dnat */
    DNAT dnat( new_egress_addr, egress_name );

    /* prepare event loop */
    EventLoop outer_event_loop;

    /* Fork */
    {
        /* Make pipe for start signal */
        auto signal_pipe = UnixDomainSocket::make_pair();

        ChildProcess container_process( "recordshell", [&]() {
                /* wait for the go signal */
                signal_pipe.second.read();

                /* bring up localhost */
                interface_ioctl( Socket( UDP ), SIOCSIFFLAGS, "lo",
                                 [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

                /* bring up veth device */
                assign_address( ingress_name, ingress_addr, egress_addr );

                /* create default route */
                rtentry route;
                zero( route );

                route.rt_gateway = egress_addr.raw_sockaddr();
                route.rt_dst = route.rt_genmask = Address().raw_sockaddr();
                route.rt_flags = RTF_UP | RTF_GATEWAY;

                SystemCall( "ioctl SIOCADDRT", ioctl( Socket( UDP ).num(), SIOCADDRT, &route ) );

                /* create DNS proxy if nameserver address is local */
                auto dns_inside = DNSProxy::maybe_proxy( nameserver,
                                                         dns_outside.udp_listener().local_addr(),
                                                         dns_outside.tcp_listener().local_addr() );

                /* Fork again after dropping root privileges */
                drop_privileges();

                /* prepare child's event loop */
                EventLoop shell_event_loop;
                {
                    int pipefd[2];
                    SystemCall( "pipe", pipe( pipefd ) );
                    FileDescriptor read_end = pipefd[ 0 ], write_end = pipefd[ 1 ];

                    shell_event_loop.add_child_process( ChildProcess( "recorded_child", bind( child_procedure, std::ref( read_end ) ) ) );

                    /* Write out input to child, NOTE: Child might choose to ignore it. */
                    write_end.write( stdin_input );
                    /* pipe endpoint gets closed when FileDescriptor goes out of scope,
                       giving phantomjs EOF and telling it that the script is over */
                }
                if ( dns_inside ) {
                    dns_inside->register_handlers( shell_event_loop );
                }
                return shell_event_loop.loop();
            }, true ); /* new network namespace */

        /* give ingress to container */
        run( { IP, "link", "set", "dev", ingress_name, "netns", to_string( container_process.pid() ) } );
        veth_devices.set_kernel_will_destroy();

        /* tell ChildProcess it's ok to proceed */
        signal_pipe.first.write( "x" );

        /* now that we have its pid, move container process to event loop */
        outer_event_loop.add_child_process( ChildProcess( move( container_process ) ) );
    }

    if ( record ) { /* use http proxy */
        /* do the actual recording in a different unprivileged child */
        outer_event_loop.add_child_process( ChildProcess( "recorder", [&]() {
                drop_privileges();

                HTTPProxy http_proxy( new_egress_addr );

                EventLoop recordr_event_loop;
                dns_outside.register_handlers( recordr_event_loop );
                http_proxy.register_handlers( recordr_event_loop, response_store_ );
                auto ret = recordr_event_loop.loop();
                response_store_.serialize_to_socket( move( socket_output ) );
                return ret;
            } ) );
    } else { /* use local proxy */
        outer_event_loop.add_child_process( ChildProcess( "localproxy", [&]() {
                drop_privileges();

                Address remote_proxy_addr( "127.0.0.1", "2222" );
                LocalProxy local_proxy( new_egress_addr, remote_proxy_addr );

                EventLoop local_proxy_loop;
                local_proxy.listen();
                dns_outside.register_handlers( local_proxy_loop );
                return local_proxy_loop.loop();
            } ) );

    }
    return outer_event_loop.loop();
}
