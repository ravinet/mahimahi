/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <net/route.h>
#include <fcntl.h>

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <chrono>
#include <thread>

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
#include "reverse_proxy.hh"
#include "pac_file.hh"
#include "vpn.hh"

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

        if ( argc < 8 ) {
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) + " directory nghttpx_path nghttpx_port nghttpx_key nghttpx_cert vpn_port path_to_dependency_file" );
        }

        /* clean directory name */
        string directory = argv[ 1 ];

        if ( directory.empty() ) {
            throw runtime_error( string( argv[ 0 ] ) + ": directory name must be non-empty" );
        }

        /* Get the application variables. */
        string nghttpx_path = string(argv[ 2 ]);
        string nghttpx_key_path = string(argv[ 4 ]);
        string nghttpx_cert_path = string(argv[ 5 ]);

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
        int vpn_port = atoi(argv[6]);
        DNAT dnat( Address(ingress_addr.ip(), vpn_port), "udp", vpn_port );

        int nghttpx_port = atoi(argv[3]);
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

                      string hostname = HTTPRequest( protobuf.request() ).get_header_value( "Host" );
                      hostname_to_ip.emplace_back( hostname, address );
                      cout << "Request first line: " << HTTPRequest( protobuf.request() ).first_line() << " IP:port: " << address.str() << endl;
                      hostname_to_ip_set.insert( make_pair(hostname, address) );
                      hostname_to_address_map.insert( make_pair(hostname, address) );
                  }
              }

              /* set up dummy interfaces */
              unsigned int interface_counter = 0;
              unsigned int squid_proxy_base_port = 3128;
              vector< Address > reverse_proxy_addresses;
              vector< Address > squid_addresses;
              vector< pair< string, Address > > hostname_to_reverse_proxy_addresses;
              vector< pair< Address, Address > > webserver_to_reverse_proxy_addresses;
              vector< pair< string, string > > hostname_to_reverse_proxy_names;
              vector< pair< string, Address > > reverse_proxy_names_to_reverse_proxy_addresses;
              for ( set< pair< string, Address > >::iterator it = hostname_to_ip_set.begin();
                  it != hostname_to_ip_set.end(); ++it) {
                  auto hostname = it->first;
                  auto address = it->second;
                  add_dummy_interface( "sharded" + to_string( interface_counter ), address );
                  uint16_t squid_port = squid_proxy_base_port + interface_counter;
                  string reverse_proxy_name = to_string( interface_counter ) + ".reverse.com";
                  Address reverse_proxy_address = Address::reverse_proxy(interface_counter + 1, nghttpx_port + interface_counter);
                  add_dummy_interface( reverse_proxy_name, reverse_proxy_address);
                  squid_addresses.push_back(Address("127.0.0.1", squid_port));
                  cout << "Hostname: " << hostname << " reverse proxy addr: " << reverse_proxy_address.str() << endl;
                  hostname_to_reverse_proxy_addresses.push_back(make_pair(hostname, reverse_proxy_address));
                  hostname_to_reverse_proxy_names.push_back(make_pair(hostname, reverse_proxy_name));
                  webserver_to_reverse_proxy_addresses.push_back(make_pair(address, reverse_proxy_address));
                  reverse_proxy_names_to_reverse_proxy_addresses.push_back(make_pair(reverse_proxy_name, reverse_proxy_address));
                  interface_counter++;
              }

              /* set up web servers */
              vector< WebServer > servers;
              for ( const auto ip_port : unique_ip_and_port ) {
                  servers.emplace_back( ip_port, working_directory, directory );
              }

              /* set up Squid Proxy */
              string path_prefix = PATH_PREFIX;
              vector< SquidProxy > squid_proxies;
              for ( const auto squid_address : squid_addresses ) {
                squid_proxies.emplace_back(squid_address, true);
                // cout << "Started Squid with port: " << squid_address.port() << endl;
                this_thread::sleep_for(chrono::milliseconds(500));
              }

              /* set up nghttpx proxies */
              string path_to_dependency_file = argv[ 7 ];
              vector< ReverseProxy > reverse_proxies;
              for ( uint16_t i = 0; i < hostname_to_reverse_proxy_addresses.size(); i++) {
                auto hostname_to_reverse_proxy = hostname_to_reverse_proxy_addresses[i];
                auto reverse_proxy_address = hostname_to_reverse_proxy.second;
                auto squid_proxy_address = squid_addresses[i];
                // auto webserver_address = webserver_to_reverse_proxy_addresses[i].first;
                cout << "ReverseProxy address: " << reverse_proxy_address.str() << " SquidProxy address: " << squid_proxy_address.str() << endl;
                reverse_proxies.emplace_back(reverse_proxy_address,
                                          squid_proxy_address,
                                          nghttpx_path,
                                          nghttpx_key_path,
                                          nghttpx_cert_path,
                                          path_to_dependency_file);
              }

              PacFile pac_file("/home/ubuntu/Sites/config_testing.pac");
              cout << hostname_to_reverse_proxy_addresses.size() << endl;
              pac_file.WriteProxies(hostname_to_reverse_proxy_addresses,
                                    hostname_to_reverse_proxy_names);

              /* set up DNS server */
              TempFile dnsmasq_hosts( "/tmp/replayshell_hosts" );
              uint8_t counter = 0;
              for ( const auto mapping : reverse_proxy_names_to_reverse_proxy_addresses ) {
              // for ( const auto mapping : hostname_to_reverse_proxy_addresses ) {
                cout << "IP: " << mapping.second.ip() << " domain: " << mapping.first << endl;
                dnsmasq_hosts.write( mapping.second.ip() + " " + to_string(counter) + ".reverse.com\n" );
                counter++;
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

              // Create a NAT to the first nameserver.
              /* set up NAT between egress and eth0 */
              NAT nat_rule( nameservers[0] );

              /* set up DNAT between tunnel and the nameserver. */
              DNAT dnat( Address(nameservers[0].ip(), 53), "udp", 53 );

              /* start dnsmasq */
              event_loop.add_child_process( start_dnsmasq( dnsmasq_args ) );

              string path_to_security_files = "/etc/openvpn/";
              VPN vpn(path_to_security_files, ingress_addr, nameservers);
              vector< string > command = vpn.start_command();
              // vector< string > command;
              // command.push_back(nghttpx_path);
              // command.push_back("-s");
              // command.push_back("-f0.0.0.0," + std::to_string(nghttpx_port));
              // command.push_back("-b127.0.0.1,3128");
              // command.push_back("-b31.13.73.7,80");
              // command.push_back(nghttpx_key_path);
              // command.push_back(nghttpx_cert_path);
              // command.push_back("bash");

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
