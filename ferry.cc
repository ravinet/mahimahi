/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <poll.h>
#include <csignal>

#include "ferry.hh"
#include "ferry_queue.hh"
#include "signalfd.hh"

using namespace std;

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

int ferry( FileDescriptor & tun,
           FileDescriptor & sibling_fd,
           DNSProxy & dns_proxy,
           ChildProcess & child_process,
           const uint64_t delay_ms )
{
    /* set up signal file descriptor */
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );
    // set up poll
    struct pollfd pollfds[ 5 ];
    pollfds[ 0 ].fd = tun.num();
    pollfds[ 0 ].events = POLLIN;

    pollfds[ 1 ].fd = sibling_fd.num();
    pollfds[ 1 ].events = POLLIN;

    pollfds[ 2 ].fd = signal_fd.raw_fd();
    pollfds[ 2 ].events = POLLIN;

    pollfds[ 3 ].fd = dns_proxy.mutable_udp_listener().raw_fd();
    pollfds[ 3 ].events = POLLIN;

    pollfds[ 4 ].fd = dns_proxy.mutable_tcp_listener().raw_fd();
    pollfds[ 4 ].events = POLLIN;

    FerryQueue delay_queue( delay_ms );

    while ( true ) {
        int wait_time = delay_queue.wait_time();
        if ( poll( pollfds, 5, wait_time ) == -1 ) {
            throw Exception( "poll" );
        }
        if ( (pollfds[ 0 ].revents
              | pollfds[ 1 ].revents
              | pollfds[ 2 ].revents
              | pollfds[ 3 ].revents
              | pollfds[ 4 ].revents )
             & (POLLERR | POLLHUP | POLLNVAL) ) { /* check for error */
            throw Exception( "poll" );
        }
        if ( pollfds[ 0 ].revents & POLLIN ) {
            /* packet FROM tun device goes to back of delay queue */
            delay_queue.read_packet( tun.read() );
        }

        if ( pollfds[ 1 ].revents & POLLIN ) {
            /* packet FROM sibling goes to tun device */
            tun.write( sibling_fd.read() );
        }

        if ( pollfds[ 2 ].revents & POLLIN ) {
            /* got a signal */
            signalfd_siginfo sig = signal_fd.read_signal();

            int return_value = handle_signal( sig, child_process );
            if ( return_value >= 0 ) {
                return return_value;
            }
        }

        if ( pollfds[ 3 ].revents & POLLIN ) {
            /* got dns request */
            dns_proxy.handle_udp();
        }
        
        if ( pollfds[ 4 ].revents & POLLIN ) {
            /* got TCP dns request */
            dns_proxy.handle_tcp();
        }

        /* packets FROM tail of delay queue go to sibling */
        delay_queue.write_packets( sibling_fd );
    }
}
