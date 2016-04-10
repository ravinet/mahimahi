/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <net/route.h>
#include <fcntl.h>

#include <vector>
#include <set>
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
#include "reverse_proxy.hh"

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

        if ( argc < 6 ) {
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) + " directory nghttpx_path nghttpx_port nghttpx_key nghttpx_cert" );
        }

        /* clean directory name */
        string directory = argv[ 1 ];

        if ( directory.empty() ) {
            throw runtime_error( string( argv[ 0 ] ) + ": directory name must be non-empty" );
        }

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
        int nghttpx_port = atoi(argv[3]);
        DNAT dnat( Address(ingress_addr.ip(), nghttpx_port), nghttpx_port );

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
                  servers.emplace_back( ip_port, working_directory, directory );
              }

              /* set up DNS server */
              TempFile dnsmasq_hosts( "/tmp/replayshell_hosts" );
              cout << "DNS Filename: " << dnsmasq_hosts.name() << endl;
              for ( const auto mapping : hostname_to_ip ) {
                  dnsmasq_hosts.write( mapping.second.ip() + " " + mapping.first + "\n" );
              }

              /* set up reverse proxies */
              string nghttpx_path = string(argv[ 2 ]);
              string nghttpx_key_path = string(argv[ 4 ]);
              string nghttpx_cert_path = string(argv[ 5 ]);

              vector< ReverseProxy > reverse_proxies;
              vector < pair<string, Address>> proxy_addresses;
              unsigned int proxy_port = 15000;
              for ( const auto ip_port : hostname_to_ip_set ) {
                  Address proxy_address("0.0.0.0", proxy_port);
                  cout << "ReverseProxy: " << proxy_address.str() << " domain: " << ip_port.first << " server address: " << ip_port.second.str() << endl;
                  proxy_addresses.emplace_back(make_pair(ip_port.first, proxy_address));
                  reverse_proxies.emplace_back(proxy_address,
                                               ip_port.second,
                                               ip_port.first,
                                               nghttpx_path,
                                               nghttpx_key_path,
                                               nghttpx_cert_path);
                  proxy_port++;
              }

              /* set up nghttp2 proxy */
              /* Command: ./nghttpx -f'0.0.0.0,10000' -b'...' [key_path] [cert_path] */
              vector< string > command;
              command.push_back(nghttpx_path);
              command.push_back("-f0.0.0.0," + std::to_string(nghttpx_port));
              for (const auto mapping : proxy_addresses ) {
                stringstream backend_args;
                backend_args << "-b" << mapping.second.ip() << "," << mapping.second.port() << ";" << mapping.first << ";proto=h2";
                command.push_back(backend_args.str());
              }
              // Default catch-all address.
              command.push_back("-b127.0.0.1,80");
              command.push_back(nghttpx_key_path);
              command.push_back(nghttpx_cert_path);

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
