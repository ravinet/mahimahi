/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <net/route.h>

#include <vector>
#include <set>

#include "util.hh"
#include "netdevice.hh"
#include "web_server.hh"
#include "system_runner.hh"
#include "socket.hh"
#include "event_loop.hh"
#include "temp_file.hh"
#include "http_response.hh"
#include "dns_server.hh"

#include "http_record.pb.h"

#include "config.h"

using namespace std;

void add_dummy_interface( const string & name, const Address & addr )
{
    run( { IP, "link", "add", name, "type", "dummy" } );

    Socket ioctl_socket( UDP );

    interface_ioctl( ioctl_socket, SIOCSIFFLAGS, name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
    interface_ioctl( ioctl_socket, SIOCSIFADDR, name,
                     [&] ( ifreq &ifr ) { ifr.ifr_addr = addr.raw_sockaddr(); } );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " directory [command...]" );
        }

        /* clean directory name */
        string directory = argv[ 1 ];

        if ( directory.empty() ) {
            throw Exception( argv[ 0 ], "directory name must be non-empty" );
        }

        /* make sure directory ends with '/' so we can prepend directory to file name for storage */
        if ( directory.back() != '/' ) {
            directory.append( "/" );
        }

        /* what command will we run inside the container? */
        vector< string > command;
        if ( argc == 2 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 2; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        /* create a new network namespace */
        SystemCall( "unshare", unshare( CLONE_NEWNET ) );

        /* bring up localhost */
        interface_ioctl( Socket( UDP ), SIOCSIFFLAGS, "lo",
                         [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

        /* provide seed for random number generator used to create apache pid files */
        srandom( time( NULL ) );

        /* collect the IPs, IPs and ports, and hostnames we'll need to serve */
        set< Address > unique_ip;
        set< Address > unique_ip_and_port;
        vector< pair< string, Address > > hostname_to_ip;

        {
            TemporarilyUnprivileged tu;
            /* would be privilege escalation if we let the user read directories or open files as root */

            const vector< string > files = list_directory_contents( directory  );

            for ( const auto filename : files ) {
                FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );

                MahimahiProtobufs::RequestResponse protobuf;
                if ( not protobuf.ParseFromFileDescriptor( fd.num() ) ) {
                    throw Exception( filename, "invalid HTTP request/response" );
                }

                const Address address( protobuf.ip(), protobuf.port() );

                unique_ip.emplace( address.ip(), 0 );
                unique_ip_and_port.emplace( address );

                hostname_to_ip.emplace_back( HTTPRequest( protobuf.request() ).get_header_value( "Host" ),
                                             address );
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
            servers.emplace_back( ip_port, directory );
        }

        /* set up DNS server */
        TempFile dnsmasq_hosts( "/tmp/replayshell_hosts" );
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
            dnsmasq_args.push_back( "-i" );
            dnsmasq_args.push_back( interface_name );
        }

        /* start dnsmasq */
        event_loop.add_child_process( ChildProcess( start_dnsmasq( dnsmasq_args ) ) );

        /* start shell */
        event_loop.add_child_process( ChildProcess( join( command ), [&]() {
                drop_privileges();

                /* restore environment and tweak bash prompt */
                environ = user_environment;
                prepend_shell_prefix( "[replay] " );

                return ezexec( command, true );
        } ) );

        return event_loop.loop();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
