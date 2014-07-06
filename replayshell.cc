/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>
#include <iostream>
#include <vector>
#include <set>

#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "signalfd.hh"
#include "netdevice.hh"
#include "web_server.hh"
#include "system_runner.hh"
#include "config.h"
#include "socket.hh"
#include "config.h"
#include "event_loop.hh"
#include "http_record.pb.h"
#include "temp_file.hh"
#include "http_response.hh"

using namespace std;
using namespace PollerShortNames;

void add_dummy_interface( const string & name, const Address & addr )
{
    run( { IP, "link", "add", name, "type", "dummy" } );

    interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
    interface_ioctl( Socket( UDP ).fd(), SIOCSIFADDR, name,
                     [&] ( ifreq &ifr ) { ifr.ifr_addr = addr.raw_sockaddr(); } );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " folder_with_recorded_content" );
        }

        /* check if user-specified storage folder exists */
        string directory = argv[1];
        /* make sure directory ends with '/' so we can prepend directory to file name for storage */
        if ( directory.back() != '/' ) {
            directory.append( "/" );
        }

        SystemCall( "unshare", unshare( CLONE_NEWNET ) );

        /* bring up localhost */
        interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "lo",
                         [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

        /* provide seed for random number generator used to create apache pid files */
        srandom( time( NULL ) );

        set< Address > unique_ip;
        set< Address > unique_ip_and_port;
        vector< pair< string, Address > > hostname_to_ip;

        {
            TemporarilyUnprivileged tu;

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
        const string user = username();
        for ( const auto ip_port : unique_ip_and_port ) {
            servers.emplace_back( ip_port, directory, user );
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
        for ( uint server_num = 0; server_num < nameservers.size(); server_num++ ) {
            add_dummy_interface( "nameserver" + to_string( server_num ), nameservers.at( server_num ) );
        }

        /* start dnsmasq to listen on 0.0.0.0:53 */
        event_loop.add_child_process( [&] () {
                SystemCall( "execl", execl( DNSMASQ, "dnsmasq", "--keep-in-foreground", "--no-resolv",
                                            "--no-hosts", "-H", dnsmasq_hosts.name().c_str(),
                                            "-C", "/dev/null", static_cast<char *>( nullptr ) ) );
                return EXIT_FAILURE;
            }, false, SIGTERM );

        /* start shell */
        event_loop.add_child_process( [&]() {
                drop_privileges();

                /* restore environment and tweak bash prompt */
                environ = user_environment;
                prepend_shell_prefix( "[replayshell] " );
                const string shell = shell_path();
                SystemCall( "execl", execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) );

                return EXIT_FAILURE;
        } );

        return event_loop.loop();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
