/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// to run arping to send packet to ingress to/from machine with ip address addr  --> arping -I ingress addr

#include <poll.h>
#include <queue>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <csignal>
#include <pwd.h>
#include <paths.h>

#include "tapdevice.hh"
#include "exception.hh"
#include "ferry_queue.hh"
#include "child_process.hh"
#include "signalfd.hh"
#include "terminal_saver.hh"

using namespace std;

/* get name of the user's shell */
string shell_path( void )
{
    struct passwd *pw = getpwuid( getuid() );
    if ( pw == nullptr ) {
        throw Exception( "getpwuid" );
    }

    string shell_path( pw->pw_shell );
    if ( shell_path.empty() ) { /* empty shell means Bourne shell */
      shell_path = _PATH_BSHELL;
    }

    return shell_path;
}

int handle_signal( const signalfd_siginfo & sig,
                   ChildProcess & child_process )
{
    switch ( sig.ssi_signo ) {
    case SIGCONT:
        /* resume child process too */
        child_process.resume();
        break;

    case SIGCHLD:
        /* make sure it's from the child process */
        /* unfortunately sig.ssi_pid is a uint32_t instead of pid_t, so need to cast */
        assert( sig.ssi_pid == static_cast<decltype(sig.ssi_pid)>( child_process.pid() ) );

        /* figure out what happened to it */
        child_process.wait();

        if ( child_process.terminated() ) {
            return child_process.exit_status();
        } else if ( !child_process.running() ) {
            /* suspend parent too */
            if ( raise( SIGSTOP ) < 0 ) {
                throw Exception( "raise" );
            }
        }
        break;

    case SIGHUP:
    case SIGTERM:
        child_process.signal( SIGHUP );

        return EXIT_SUCCESS;
        break;
    default:
        throw Exception( "unknown signal" );
    }

    return -1;
}

int ferry( const FileDescriptor & tap,
           const FileDescriptor & sibling_fd,
           ChildProcess & child_process,
           const uint64_t delay_ms )
{
    /* set up signal file descriptor */
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );

    // set up poll
    struct pollfd pollfds[ 3 ];
    pollfds[ 0 ].fd = tap.num();
    pollfds[ 0 ].events = POLLIN;

    pollfds[ 1 ].fd = sibling_fd.num();
    pollfds[ 1 ].events = POLLIN;

    pollfds[ 2 ].fd = signal_fd.fd().num();
    pollfds[ 2 ].events = POLLIN;

    FerryQueue delay_queue( delay_ms );

    while ( true ) {
        int wait_time = delay_queue.wait_time();
        
        if ( poll( pollfds, 3, wait_time ) == -1 ) {
            throw Exception( "poll" );
        }

        if ( (pollfds[ 0 ].revents
              | pollfds[ 1 ].revents
              | pollfds[ 2 ].revents )
             & (POLLERR | POLLHUP | POLLNVAL) ) { /* check for error */
            throw Exception( "poll" );
        }

        if ( pollfds[ 0 ].revents & POLLIN ) {
            /* packet FROM tap device goes to back of delay queue */
            delay_queue.read_packet( tap.read() );
        }

        if ( pollfds[ 1 ].revents & POLLIN ) {
            /* packet FROM sibling goes to tap device */
            tap.write( sibling_fd.read() );
        }

        if ( pollfds[ 2 ].revents & POLLIN ) {
            /* got a signal */
            signalfd_siginfo sig = signal_fd.read_signal();

            int return_value = handle_signal( sig, child_process );
            if ( return_value >= 0 ) {
                return return_value;
            }
        }

        /* packets FROM tail of delay queue go to sibling */
        delay_queue.write_packets( sibling_fd );
    }
}

int main( void )
{
    TerminalSaver saved_state; /* terminal will be restored when object
                                  destroyed, even if shell exits uncleanly */

    try {
        /* make pair of connected sockets */
        int pipes[ 2 ];
        if ( socketpair( AF_UNIX, SOCK_DGRAM, 0, pipes ) < 0 ) {
            throw Exception( "socketpair" );
        }
        FileDescriptor egress_socket( pipes[ 0 ], "socketpair" ),
            ingress_socket( pipes[ 1 ], "socketpair" );

        /* Fork */
        ChildProcess container_process( [&](){
                /* Unshare network namespace */
                if ( unshare( CLONE_NEWNET ) == -1 ) {
                    throw Exception( "unshare" );
                }

                TapDevice ingress_tap( "ingress" );

                /* Fork again */
                ChildProcess bash_process( [](){
                        const string shell = shell_path();
                        if ( execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) < 0 ) {
                            throw Exception( "execl" );
                        }
                        return EXIT_FAILURE;
                    } );

                return ferry( ingress_tap.fd(), egress_socket, bash_process, 2500 );
            } );

        TapDevice egress_tap( "egress" );
        return ferry( egress_tap.fd(), ingress_socket, container_process, 2500 );
    } catch ( const Exception & e ) {
        e.die();
    }

    return EXIT_SUCCESS;
}
