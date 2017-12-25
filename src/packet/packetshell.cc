/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <chrono>
#include <iostream>

#include <sys/socket.h>
#include <net/route.h>

#include "packetshell.hh"
#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "dns_server.hh"
#include "timestamp.hh"
#include "exception.hh"
#include "bindworkaround.hh"
#include "config.h"
#include "vpn.hh"

using namespace std;
using namespace PollerShortNames;

template <class FerryQueueType>
PacketShell<FerryQueueType>::PacketShell( const std::string & device_prefix, char ** const user_environment )
    : user_environment_( user_environment ),
      egress_ingress( two_unassigned_addresses( get_mahimahi_base() ) ),
      nameserver_( first_nameserver() ),
      egress_tun_( device_prefix + "-" + to_string( getpid() ) , egress_addr(), ingress_addr() ),
      dns_outside_( egress_addr(), nameserver_, nameserver_ ),
      nat_rule_( ingress_addr() ),
      dnat_rule_(),
      pipe_( UnixDomainSocket::make_pair() ),
      event_loop_()
{
    /* make sure environment has been cleared */
    if ( environ != nullptr ) {
        throw runtime_error( "PacketShell: environment was not cleared" );
    }

    /* initialize base timestamp value before any forking */
    initial_timestamp();
}

template <class FerryQueueType>
PacketShell<FerryQueueType>::PacketShell( const std::string & device_prefix, char ** const user_environment, int destination_port )
    : user_environment_( user_environment ),
      egress_ingress( two_unassigned_addresses( get_mahimahi_base() ) ),
      nameserver_( first_nameserver() ),
      egress_tun_( device_prefix + "-" + to_string( getpid() ) , egress_addr(), ingress_addr() ),
      dns_outside_( egress_addr(), nameserver_, nameserver_ ),
      nat_rule_(),
      dnat_rule_( Address(ingress_addr().ip(), destination_port), "udp", destination_port ),
      pipe_( UnixDomainSocket::make_pair() ),
      event_loop_()
{
    /* make sure environment has been cleared */
    if ( environ != nullptr ) {
        throw runtime_error( "PacketShell: environment was not cleared" );
    }

    /* initialize base timestamp value before any forking */
    initial_timestamp();
}

template <class FerryQueueType>
template <typename... Targs>
void PacketShell<FerryQueueType>::start_uplink_and_forward_packets_with_nameserver
                                  ( const string & shell_prefix,
                                    const int destination_port,
                                    const Address nameserver_address,
                                    const vector< string > & command,
                                    Targs&&... Fargs )
{
    /* g++ bug 55914 makes this hard before version 4.9 */
    BindWorkAround::bind<FerryQueueType, Targs&&...> ferry_maker( forward<Targs>( Fargs )... );

    /*
      This is a replacement for expanding the parameter pack
      inside the lambda, e.g.:

    auto ferry_maker = [&]() {
        return FerryQueueType( forward<Targs>( Fargs )... );
    };
    */

    cout << "ingress: " << ingress_addr().str() << " egress: " << egress_addr().str() << endl;
    /* Forward the packets to the specified port. */
    // DNAT dnat( Address(ingress_addr().ip(), destination_port), "udp", destination_port );
    // DNATWithPostrouting dnat_with_postrouting( Address(ingress_addr().ip(), destination_port), "udp", destination_port );
    cout << "destination_port: " << to_string(destination_port) << endl;

    /* Fork */
    event_loop_.add_special_child_process( 77, "packetshell", [&]() {
            TunDevice ingress_tun( "ingress", ingress_addr(), egress_addr() );

            /* bring up localhost */
            interface_ioctl( SIOCSIFFLAGS, "lo",
                             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

            /* create default route */
            rtentry route;
            zero( route );

            route.rt_gateway = egress_addr().to_sockaddr();
            route.rt_dst = route.rt_genmask = Address().to_sockaddr();
            route.rt_flags = RTF_UP | RTF_GATEWAY;

            SystemCall( "ioctl SIOCADDRT", ioctl( UDPSocket().fd_num(), SIOCADDRT, &route ) );

            Ferry inner_ferry;

            /* dnsmasq doesn't distinguish between UDP and TCP forwarding nameservers,
               so use a DNSProxy that listens on the same UDP and TCP port */

            UDPSocket dns_udp_listener;
            dns_udp_listener.bind( ingress_addr() );

            TCPSocket dns_tcp_listener;
            dns_tcp_listener.bind( dns_udp_listener.local_address() );

            DNSProxy dns_inside_ { move( dns_udp_listener ), move( dns_tcp_listener ),
                    dns_outside_.udp_listener().local_address(),
                    dns_outside_.tcp_listener().local_address() };

            cout << "udp_listener.local_address(): " << dns_outside_.udp_listener().local_address().str() << endl;

            dns_inside_.register_handlers( inner_ferry );

            /* run dnsmasq as local caching nameserver */
            inner_ferry.add_child_process( start_dnsmasq( {
                        "-S", dns_inside_.udp_listener().local_address().str( "#" ) } ) );

            /* For debugging purposes */
            // vector< string > vpn_command;
            // vpn_command.push_back("bash");

            /* Setup proper port forwarding to the nameserver. */
            nameserver_address.str();
            // if ( ( geteuid() == 0 ) or ( getegid() == 0 ) ) {
            //   cout << "Packetshell: not root" << endl;
            // }
            // nameserver_address.str();
            DNATWithPostrouting dnat_with_postrouting( nameserver_address, "udp", 53 );

            /* Fork again after dropping root privileges */
            drop_privileges();

            string path_to_security_files = "/etc/openvpn/";
            const vector< Address > nameservers = { nameserver_address };
            VPN vpn(path_to_security_files, ingress_addr(), nameservers);
            vector< string > vpn_command = vpn.start_command();
            command.empty(); // so that the compiler doesn't complain about unused variable.

            /* restore environment */
            environ = user_environment_;


            /* set MAHIMAHI_BASE if not set already to indicate outermost container */
            SystemCall( "setenv", setenv( "MAHIMAHI_BASE",
                                          egress_addr().ip().c_str(),
                                          false /* don't override */ ) );

            inner_ferry.add_child_process( join( vpn_command ), [&]() {
                    /* tweak bash prompt */
                    prepend_shell_prefix( shell_prefix );

                    return ezexec( vpn_command, true );
                } );

            /* allow downlink to write directly to inner namespace's TUN device */
            pipe_.first.send_fd( ingress_tun );

            FerryQueueType uplink_queue { ferry_maker() };
            return inner_ferry.loop( uplink_queue, ingress_tun, egress_tun_ );
        }, true );  /* new network namespace */

}

template <class FerryQueueType>
template <typename... Targs>
void PacketShell<FerryQueueType>::start_uplink_and_forward_packets
                                  ( const string & shell_prefix,
                                    const int destination_port,
                                    const vector< string > & command,
                                    Targs&&... Fargs )
{
    /* g++ bug 55914 makes this hard before version 4.9 */
    BindWorkAround::bind<FerryQueueType, Targs&&...> ferry_maker( forward<Targs>( Fargs )... );

    /*
      This is a replacement for expanding the parameter pack
      inside the lambda, e.g.:

    auto ferry_maker = [&]() {
        return FerryQueueType( forward<Targs>( Fargs )... );
    };
    */

    cout << "ingress: " << ingress_addr().str() << " egress: " << egress_addr().str() << endl;
    /* Forward the packets to the specified port. */
    DNAT dnat( Address(ingress_addr().ip(), destination_port), "udp", destination_port );

    /* Fork */
    event_loop_.add_special_child_process( 77, "packetshell", [&]() {
            TunDevice ingress_tun( "ingress", ingress_addr(), egress_addr() );

            /* bring up localhost */
            interface_ioctl( SIOCSIFFLAGS, "lo",
                             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

            /* create default route */
            rtentry route;
            zero( route );

            route.rt_gateway = egress_addr().to_sockaddr();
            route.rt_dst = route.rt_genmask = Address().to_sockaddr();
            route.rt_flags = RTF_UP | RTF_GATEWAY;

            SystemCall( "ioctl SIOCADDRT", ioctl( UDPSocket().fd_num(), SIOCADDRT, &route ) );

            Ferry inner_ferry;

            /* dnsmasq doesn't distinguish between UDP and TCP forwarding nameservers,
               so use a DNSProxy that listens on the same UDP and TCP port */

            UDPSocket dns_udp_listener;
            dns_udp_listener.bind( ingress_addr() );

            TCPSocket dns_tcp_listener;
            dns_tcp_listener.bind( dns_udp_listener.local_address() );

            DNSProxy dns_inside_ { move( dns_udp_listener ), move( dns_tcp_listener ),
                    dns_outside_.udp_listener().local_address(),
                    dns_outside_.tcp_listener().local_address() };

            cout << "udp_listener.local_address(): " << dns_outside_.udp_listener().local_address().str() << endl;

            dns_inside_.register_handlers( inner_ferry );

            /* run dnsmasq as local caching nameserver */
            inner_ferry.add_child_process( start_dnsmasq( {
                        "-S", dns_inside_.udp_listener().local_address().str( "#" ) } ) );

            /* Fork again after dropping root privileges */
            drop_privileges();

            /* restore environment */
            environ = user_environment_;

            /* set MAHIMAHI_BASE if not set already to indicate outermost container */
            SystemCall( "setenv", setenv( "MAHIMAHI_BASE",
                                          egress_addr().ip().c_str(),
                                          false /* don't override */ ) );

            inner_ferry.add_child_process( join( command ), [&]() {
                    /* tweak bash prompt */
                    prepend_shell_prefix( shell_prefix );

                    return ezexec( command, true );
                } );

            /* allow downlink to write directly to inner namespace's TUN device */
            pipe_.first.send_fd( ingress_tun );

            FerryQueueType uplink_queue { ferry_maker() };
            return inner_ferry.loop( uplink_queue, ingress_tun, egress_tun_ );
        }, true );  /* new network namespace */

}

template <class FerryQueueType>
template <typename... Targs>
void PacketShell<FerryQueueType>::start_uplink( const string & shell_prefix,
                                                const vector< string > & command,
                                                Targs&&... Fargs )
{
    /* g++ bug 55914 makes this hard before version 4.9 */
    BindWorkAround::bind<FerryQueueType, Targs&&...> ferry_maker( forward<Targs>( Fargs )... );

    /*
      This is a replacement for expanding the parameter pack
      inside the lambda, e.g.:

    auto ferry_maker = [&]() {
        return FerryQueueType( forward<Targs>( Fargs )... );
    };
    */
    cout << "ingress: " << ingress_addr().str() << " egress: " << egress_addr().str() << endl;

    /* Fork */
    event_loop_.add_special_child_process( 77, "packetshell", [&]() {
            TunDevice ingress_tun( "ingress", ingress_addr(), egress_addr() );

            /* bring up localhost */
            interface_ioctl( SIOCSIFFLAGS, "lo",
                             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

            /* create default route */
            rtentry route;
            zero( route );

            route.rt_gateway = egress_addr().to_sockaddr();
            route.rt_dst = route.rt_genmask = Address().to_sockaddr();
            route.rt_flags = RTF_UP | RTF_GATEWAY;

            SystemCall( "ioctl SIOCADDRT", ioctl( UDPSocket().fd_num(), SIOCADDRT, &route ) );

            Ferry inner_ferry;

            /* dnsmasq doesn't distinguish between UDP and TCP forwarding nameservers,
               so use a DNSProxy that listens on the same UDP and TCP port */

            UDPSocket dns_udp_listener;
            dns_udp_listener.bind( ingress_addr() );

            TCPSocket dns_tcp_listener;
            dns_tcp_listener.bind( dns_udp_listener.local_address() );

            DNSProxy dns_inside_ { move( dns_udp_listener ), move( dns_tcp_listener ),
                    dns_outside_.udp_listener().local_address(),
                    dns_outside_.tcp_listener().local_address() };

            dns_inside_.register_handlers( inner_ferry );

            cout << "udp_listener.local_address(): " << dns_outside_.udp_listener().local_address().str() << endl;

            /* run dnsmasq as local caching nameserver */
            inner_ferry.add_child_process( start_dnsmasq( {
                        "-S", dns_inside_.udp_listener().local_address().str( "#" ) } ) );

            /* Fork again after dropping root privileges */
            drop_privileges();

            /* restore environment */
            environ = user_environment_;

            /* set MAHIMAHI_BASE if not set already to indicate outermost container */
            SystemCall( "setenv", setenv( "MAHIMAHI_BASE",
                                          egress_addr().ip().c_str(),
                                          false /* don't override */ ) );

            inner_ferry.add_child_process( join( command ), [&]() {
                    /* tweak bash prompt */
                    prepend_shell_prefix( shell_prefix );

                    return ezexec( command, true );
                } );

            /* allow downlink to write directly to inner namespace's TUN device */
            pipe_.first.send_fd( ingress_tun );

            FerryQueueType uplink_queue { ferry_maker() };
            return inner_ferry.loop( uplink_queue, ingress_tun, egress_tun_ );
        }, true );  /* new network namespace */
}

template <class FerryQueueType>
template <typename... Targs>
void PacketShell<FerryQueueType>::start_downlink( Targs&&... Fargs )
{
    /* g++ bug 55914 makes this hard before version 4.9 */
    BindWorkAround::bind<FerryQueueType, Targs&&...> ferry_maker( forward<Targs>( Fargs )... );

    /*
      This is a replacement for expanding the parameter pack
      inside the lambda, e.g.:

    auto ferry_maker = [&]() {
        return FerryQueueType( forward<Targs>( Fargs )... );
    };
    */

    event_loop_.add_special_child_process( 77, "downlink", [&] () {
            drop_privileges();

            /* restore environment */
            environ = user_environment_;

            /* downlink packets go to inner namespace's TUN device */
            FileDescriptor ingress_tun = pipe_.second.recv_fd();

            Ferry outer_ferry;

            dns_outside_.register_handlers( outer_ferry );

            FerryQueueType downlink_queue { ferry_maker() };
            return outer_ferry.loop( downlink_queue, egress_tun_, ingress_tun );
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

    /* ferry ready to write datagram -> send to sibling's tun device */
    add_action( Poller::Action( sibling, Direction::Out,
                                [&] () {
                                    ferry_queue.write_packets( sibling );
                                    return ResultType::Continue;
                                },
                                [&] () { return ferry_queue.pending_output(); } ) );

    /* exit if finished */
    add_action( Poller::Action( sibling, Direction::Out,
                                [&] () {
                                    return Result( ResultType::Exit, 77 );
                                },
                                [&] () { return ferry_queue.finished(); } ) );

    return internal_loop( [&] () { return ferry_queue.wait_time(); } );
}

struct TemporaryEnvironment
{
    TemporaryEnvironment( char ** const env )
    {
        if ( environ != nullptr ) {
            throw runtime_error( "TemporaryEnvironment: cannot be entered recursively" );
        }
        environ = env;
    }

    ~TemporaryEnvironment()
    {
        environ = nullptr;
    }
};

template <class FerryQueueType>
Address PacketShell<FerryQueueType>::get_mahimahi_base( void ) const
{
    /* temporarily break our security rule of not looking
       at the user's environment before dropping privileges */
    TemporarilyUnprivileged tu;
    TemporaryEnvironment te { user_environment_ };

    const char * const mahimahi_base = getenv( "MAHIMAHI_BASE" );
    if ( not mahimahi_base ) {
        return Address();
    }

    return Address( mahimahi_base, 0 );
}
