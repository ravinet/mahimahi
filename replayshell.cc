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
#include "http_header.hh"

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

/* return hostname by iterating through headers */
string get_host( HTTP_Record::reqrespair & current_record )
{
    for ( int i = 0; i < current_record.req().headers_size(); i++ ) {
        HTTPHeader current_header( current_record.req().headers(i) );
        if ( current_header.key() == "Host" ) {
            return current_header.value().substr( 0, current_header.value().find( "\r\n" ) );
        }
    }

    throw Exception( "replayshell", "No Host Header in Recorded File" );
}

int main( int argc, char *argv[] )
{
    try {
        string user( getenv( "USER" ) ); /* XXX we need to fix this */
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

        if ( not check_folder_existence( directory ) ) { /* folder does not exist */
            throw Exception( "replayshell", "folder with recorded content does not exist" );
        }

        SystemCall( "unshare", unshare( CLONE_NEWNET ) );

        /* bring up localhost */
        interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "lo",
                         [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

        /* provide seed for random number generator used to create apache pid files */
        srandom( time( NULL ) );

        /* dnsmasq host mapping file */
        TempFile dnsmasq_hosts( "hosts" );

        vector< string > files = list_directory_contents( directory  );
        set< Address > unique_addrs;
        set< string > unique_ips;
        vector< WebServer > servers;

        unsigned int interface_counter = 0;

        for ( unsigned int i = 0; i < files.size(); i++ ) {
            FileDescriptor response( SystemCall( "open", open( files[i].c_str(), O_RDONLY ) ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( response.num() );
            Address current_addr( current_record.ip(), current_record.port() );
            auto result1 = unique_ips.emplace( current_addr.ip() );
            if ( result1.second ) { /* new ip */
                add_dummy_interface( "sharded" + to_string( interface_counter ), current_addr );
                interface_counter++;
            }

            /* add entry to dnsmasq host mapping file */
            string entry_host = get_host( current_record );
            dnsmasq_hosts.write( current_addr.ip() + " " +entry_host + "\n" );

            auto result2 = unique_addrs.emplace( current_addr );
            if ( result2.second ) { /* new address */
                servers.emplace_back( current_addr, directory, user );
            }
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
    return EXIT_SUCCESS;
}
