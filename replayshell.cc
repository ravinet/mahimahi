/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>
#include <iostream>
#include <vector>
#include <set>

#include "util.hh"
#include "get_address.hh"
#include "address.hh"
#include "signalfd.hh"
#include "netdevice.hh"
#include "web_server.hh"
#include "system_runner.hh"
#include "config.h"
#include "socket.hh"
#include "config.h"
#include "poller.hh"
#include "http_record.pb.h"
#include "temp_file.hh"
#include "http_header.hh"

using namespace std;
using namespace PollerShortNames;

int eventloop( ChildProcess & child_process )
{
    /* set up signal file descriptor */
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );

    Poller poller;

    poller.add_action( Poller::Action( signal_fd.fd(), Direction::In,
                                       [&] () {
                                           return handle_signal( signal_fd.read_signal(),
                                                                 child_process );
                                       } ) );

    while ( true ) {
        auto poll_result = poller.poll( 60000 );
        if ( poll_result.result == Poller::Result::Type::Exit ) {
            /* call destructor for each web server created */
            return poll_result.exit_status;
        }
    }
}

void add_dummy_interface( const string & name, const Address & addr )
{

    run( { IP, "link", "add", name.c_str(), "type", "dummy" } );

    interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, name.c_str(),
             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
    interface_ioctl( Socket( UDP ).fd(), SIOCSIFADDR, name.c_str(),
         [&] ( ifreq &ifr )
         { ifr.ifr_addr = addr.raw_sockaddr(); } );
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
        TempFile dnsmasq( "", "hosts" );

        vector< string > files;
        list_files( directory, files );
        set< Address > unique_addrs;
        set< string > unique_ips;
        vector< WebServer > servers;
        servers.reserve( files.size() );
        unsigned int interface_counter = 0;

        for ( unsigned int i = 0; i < files.size(); i++ ) {
            int fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( fd );
            Address current_addr( current_record.ip(), current_record.port() );
            auto result1 = unique_ips.emplace( current_addr.ip() );
            if ( result1.second ) { /* new ip */
                add_dummy_interface( "sharded" + to_string( interface_counter ), current_addr );
                interface_counter++;
                /* add entry to dnsmasq host mapping file */
                string entry_host = get_host( current_record );
                dnsmasq.append( current_addr.ip() + " " +entry_host + "\n" );
            }

            auto result2 = unique_addrs.emplace( current_addr );
            if ( result2.second ) { /* new address */
                servers.emplace_back( current_addr, directory );
            }
            SystemCall( "close", close( fd ) );
        }
        /* start dnsmasq with created host mapping file */
        run( { DNSMASQ, "-i", "lo", "-h", "-H", dnsmasq.name() } );

        ChildProcess bash_process( [&]() {
                drop_privileges();

                /* restore environment and tweak bash prompt */
                environ = user_environment;
                prepend_shell_prefix( "[replayshell] " );
                const string shell = shell_path();
                SystemCall( "execl", execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) );
                return EXIT_FAILURE;
        } );
        return eventloop( bash_process );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
