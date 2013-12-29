/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>
#include <iostream>
#include <vector>
#include <dirent.h>

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

using namespace std;
using namespace PollerShortNames;

int eventloop( ChildProcess & child_process )
{
    /* set up signal file descriptor */
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );

    Poller poller;

    /* we get signal -> main screen turn on -> handle signal */
    poller.add_action( Poller::Action( signal_fd.fd(), Direction::In,
                                       [&] () {
                                           return handle_signal( signal_fd.read_signal(),
                                                                 child_process );
                                       } ) );

    while ( true ) {
        auto poll_result = poller.poll( 60000 );
        if ( poll_result.result == Poller::Result::Type::Exit ) {
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

void list_files( const string & dir, vector< string > & files )
{
    DIR *dp;
    struct dirent *dirp;

    if( ( dp  = opendir( dir.c_str() ) ) == NULL ) {
        throw Exception( "opendir" );
    }

    while ( ( dirp = readdir( dp ) ) != NULL ) {
        if ( string( dirp->d_name ) != "." and string( dirp->d_name ) != ".." ) {
            files.push_back( string( dirp->d_name ) );
        }
    }
    SystemCall( "closedir", closedir( dp ) );
}


int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        /* set egress and ingress ip addresses */
        Interfaces interfaces;

        auto egress_octet = interfaces.first_unassigned_address( 1 );
        auto ingress_octet = interfaces.first_unassigned_address( egress_octet.second + 1 );
        Address egress_addr = egress_octet.first, ingress_addr = ingress_octet.first;

        SystemCall( "unshare", unshare( CLONE_NEWNET ) );

        /* bring up localhost */
        interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "lo",
                         [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

        /* create and bring up two dummy interfaces */
        /* want to: go through recorded folder and for each file, check ip/port...if unique ip, make dummy, if unique ip/port, start web server ...consider dirent.h*/
        /* protobuf files are files with the serializedstring protobuf written to them */

        vector< string > files;
        list_files( "recordtest", files );
        for ( unsigned int i = 0; i < files.size(); i++ ) {
            cout << files[ i ] << endl;
        }

        add_dummy_interface( "dumb00", egress_addr );
        add_dummy_interface( "dumb11", ingress_addr );

        srandom( time( NULL ) );

        WebServer apache1( Address( "100.64.0.1", 80 ) );
        WebServer apache2( Address( "100.64.0.2", 80 ) );

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
