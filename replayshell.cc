/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>
#include <iostream>

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

        run( { IP, "link", "add", "dumb00", "type", "dummy" } );
        run( { IP, "link", "add", "dumb11", "type", "dummy" } );
        interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "dumb00",
                 [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
        interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "dumb11",
                 [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
        interface_ioctl( Socket( UDP ).fd(), SIOCSIFADDR, "dumb00",
             [&] ( ifreq &ifr )
             { ifr.ifr_addr = egress_addr.raw_sockaddr(); } );
        interface_ioctl( Socket( UDP ).fd(), SIOCSIFADDR, "dumb11",
             [&] ( ifreq &ifr )
             { ifr.ifr_addr = ingress_addr.raw_sockaddr(); } );

        srandom( time( NULL ) );

        WebServer apache1( "Listen 100.64.0.1:80", 80);
        WebServer apache2( "Listen 100.64.0.2:80", 80);

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
