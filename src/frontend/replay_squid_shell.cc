/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <net/route.h>
#include <fcntl.h>

#include <vector>
#include <set>
#include <map>
#include <sstream>

#include "util.hh"
#include "netdevice.hh"
#include "web_server.hh"
#include "system_runner.hh"
#include "socket.hh"
#include "event_loop.hh"
#include "temp_file.hh"
#include "http_response.hh"
#include "dns_server.hh"
#include "exception.hh"
#include "interfaces.hh"
#include "nat.hh"
#include "socketpair.hh"
#include "squid_proxy.hh"

#include "http_record.pb.h"

#include "config.h"

using namespace std;

void add_dummy_interface( const string & name, const Address & addr )
{
    run( { IP, "link", "add", name, "type", "dummy" } );

    interface_ioctl( SIOCSIFFLAGS, name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
    interface_ioctl( SIOCSIFADDR, name,
                     [&] ( ifreq &ifr ) { ifr.ifr_addr = addr.to_sockaddr(); } );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 2 ) {
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) + " directory" );
        }

        /* clean directory name */
        string directory = argv[ 1 ];

        if ( directory.empty() ) {
            throw runtime_error( string( argv[ 0 ] ) + ": directory name must be non-empty" );
        }

        /* Get the application variables. */
        /* make sure directory ends with '/' so we can prepend directory to file name for storage */
        if ( directory.back() != '/' ) {
            directory.append( "/" );
        }

        /* get working directory */
        const string working_directory { get_working_directory() };

        /* chdir to result of getcwd just in case */
        SystemCall( "chdir", chdir( working_directory.c_str() ) );

        /* set egress and ingress ip addresses */
        Address egress_addr, ingress_addr;
        tie( egress_addr, ingress_addr ) = two_unassigned_addresses();

        /* make pair of devices */
        string egress_name = "veth-" + to_string( getpid() ), ingress_name = "veth-i" + to_string( getpid() );
        VirtualEthernetPair veth_devices( egress_name, ingress_name );
        cout << "egress name: " << egress_name << " ingress name: " << ingress_name << endl;

        /* bring up egress */
        assign_address( egress_name, egress_addr, ingress_addr );

        /* set up NAT between egress and eth0 */
        NAT nat_rule( ingress_addr );

        /* set up DNAT between eth0 to ingress address. */
        DNAT dnat( Address(ingress_addr.ip(), 3128), 3128 );

        EventLoop outer_event_loop;
        
        /* Fork */
        {
          /* Make pipe for start signal */
          auto pipe = UnixDomainSocket::make_pair();

          ChildProcess container_process( "proxy-replayshell", [&]() {
              pipe.second.read();

              /* bring up localhost */
              interface_ioctl( SIOCSIFFLAGS, "lo",
                               [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

              /* bring up veth device */
              assign_address( ingress_name, ingress_addr, egress_addr );

              /* create default route */
              rtentry route;
              zero( route );

              route.rt_gateway = egress_addr.to_sockaddr();
              route.rt_dst = route.rt_genmask = Address().to_sockaddr();
              route.rt_flags = RTF_UP | RTF_GATEWAY;

              SystemCall( "ioctl SIOCADDRT", ioctl( UDPSocket().fd_num(), SIOCADDRT, &route ) );

              /* provide seed for random number generator used to create apache pid files */
              srandom( time( NULL ) );

              /* collect the IPs, IPs and ports, and hostnames we'll need to serve */
              set< Address > unique_ip;
              set< Address > unique_ip_and_port;
              vector< pair< string, Address > > hostname_to_ip;
              set< pair< string, Address > > hostname_to_ip_set;
              map<string, Address> hostname_to_address_map;

              {
                  TemporarilyUnprivileged tu;
                  /* would be privilege escalation if we let the user read directories or open files as root */

                  const vector< string > files = list_directory_contents( directory  );

                  for ( const auto filename : files ) {
                      FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );

                      MahimahiProtobufs::RequestResponse protobuf;
                      if ( not protobuf.ParseFromFileDescriptor( fd.fd_num() ) ) {
                          throw runtime_error( filename + ": invalid HTTP request/response" );
                      }

                      const Address address( protobuf.ip(), protobuf.port() );

                      unique_ip.emplace( address.ip(), 0 );
                      unique_ip_and_port.emplace( address );

                      hostname_to_ip.emplace_back( HTTPRequest( protobuf.request() ).get_header_value( "Host" ),
                                                   address );
                      cout << "Request first line: " << HTTPRequest( protobuf.request() ).first_line() << " IP:port: " << address.str() << endl;
                      hostname_to_ip_set.insert( make_pair(HTTPRequest( protobuf.request() ).get_header_value( "Host" ),
                                              address) );
                      hostname_to_address_map.insert( make_pair(HTTPRequest( protobuf.request() ).get_header_value( "Host" ),
                                              address) );
                  }
              }

              /* set up dummy interfaces */
              unsigned int interface_counter = 0;
              for ( const auto ip : unique_ip ) {
                  add_dummy_interface( "sharded" + to_string( interface_counter ), ip );
                  interface_counter++;
              }

              /* set up web servers */
              vector< WebServer > servers;
              for ( const auto ip_port : unique_ip_and_port ) {
                  auto port = ip_port.port();
                  if (port == 443) {
                      servers.emplace_back( ip_port, working_directory, directory );
                  } else {
                      servers.emplace_back( ip_port, working_directory, directory );
                  }
              }

              // /* set up dummy interfaces */
              // vector< pair<Address, Address> > reverse_proxy_addresses;
              // vector< pair<string, Address> > hostname_to_reverse_proxy_mapping;
              // unsigned int reverse_proxy_counter = 2;
              // for ( const auto hostname_to_address : hostname_to_ip_set ) {
              //     Address proxy_address = Address::reverse_proxy(reverse_proxy_counter, hostname_to_address.second.port());
              //     add_dummy_interface( "reverse_proxy" + to_string( reverse_proxy_counter ), proxy_address );
              //     reverse_proxy_addresses.push_back(make_pair(hostname_to_address.second, proxy_address));
              //     hostname_to_reverse_proxy_mapping.push_back(
              //         make_pair(hostname_to_address.first, proxy_address));
              //     reverse_proxy_counter++;
              // }

              string path_prefix = PATH_PREFIX;

              /* set up DNS server */
              TempFile dnsmasq_hosts( "/tmp/replayshell_hosts" );
              cout << "DNS Filename: " << dnsmasq_hosts.name() << endl;
              for ( const auto mapping : hostname_to_ip ) {
                dnsmasq_hosts.write( mapping.second.ip() + " " + mapping.first + "\n" );
              }

              /* initialize event loop */
              EventLoop event_loop;

              /* create dummy interface for each nameserver */
              vector< Address > nameservers = all_nameservers();
              vector< string > dnsmasq_args = { "-H", dnsmasq_hosts.name() };

              for ( unsigned int server_num = 0; server_num < nameservers.size(); server_num++ ) {
                  const string interface_name = "nameserver" + to_string( server_num );
                  add_dummy_interface( interface_name, nameservers.at( server_num ) );
              }

              /* start dnsmasq */
              event_loop.add_child_process( start_dnsmasq( dnsmasq_args ) );

              Address squid_address("0.0.0.0", 3128);
              SquidProxy squid_proxy(squid_address, false);
              vector< string > command = squid_proxy.command();

              /* start shell */
              event_loop.add_child_process( join( command ), [&]() {
                      drop_privileges();

                      /* restore environment and tweak bash prompt */
                      environ = user_environment;
                      prepend_shell_prefix( "[proxy-replay] " );

                      return ezexec( command, true );
              } );

            return event_loop.loop();

          }, true); /* new network namespace */

          /* give ingress to container */
          run( { IP, "link", "set", "dev", ingress_name, "netns", to_string( container_process.pid() ) } );
          veth_devices.set_kernel_will_destroy();

          /* tell ChildProcess it's ok to proceed */
          pipe.first.write( "x" );

          /* now that we have its pid, move container process to event loop */
          outer_event_loop.add_child_process( move( container_process ) );
        }

        return outer_event_loop.loop();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
