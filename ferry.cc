/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <poll.h>
#include <csignal>
#include <utility>
#include <iostream>
#include <string>
#include <thread>

#include "ferry.hh"
#include "ferry_queue.hh"
#include "signalfd.hh"

using namespace std;

void service_udp_request( Socket & server_socket, const pair< Address, string > request, const Address & connectaddr )
{
    try {
        Socket outgoing_socket( SocketType::UDP );
        outgoing_socket.connect( connectaddr );

        /* send request to local dns server */
        outgoing_socket.write( request.second );

        /* wait up to 60 seconds for a reply */
        struct pollfd pollfds;
        pollfds.fd = outgoing_socket.raw_fd();
        pollfds.events = POLLIN;

        if ( poll( &pollfds, 1, 60000 ) < 0 ) {
            throw Exception( "poll" );
        }

        if ( pollfds.revents & POLLIN ) {
            /* read response, then send back to client */
            server_socket.sendto( request.first, outgoing_socket.read() );
        }
    } catch ( const Exception & e ) {
        cerr.flush();
        e.perror();
        return;
    }

    cerr.flush();
    return;
}

void service_tcp_request( Socket & server_socket, const Address & connectaddr )
{
    try {
        Socket outgoing_socket( SocketType::TCP );
        outgoing_socket.connect( connectaddr );

        struct pollfd pollfds[ 2 ];
        pollfds[ 0 ].fd = outgoing_socket.raw_fd();
        pollfds[ 1 ].fd = server_socket.raw_fd();

        /* process requests until either socket is idle for 60 seconds or until EOF */
        while( true ) {
            pollfds[ 0 ].events = POLLIN;
            pollfds[ 1 ].events = POLLIN;

            if ( poll( pollfds, 2, 60000 ) < 0 ) {
                throw Exception( "poll" );
            }

            /* if response comes from local dns server, write back to source of request */
            if ( pollfds[ 0 ].revents & POLLIN ) {
                /* read response, then send back to client */
                string buffer = outgoing_socket.read();
                if ( buffer.empty() ) {
                    break;
                }
                server_socket.write( buffer );
            } else if ( pollfds[ 1 ].revents & POLLIN ) {
                /* read request, then send to local dns server */
                string buffer = server_socket.read();
                if ( buffer.empty() ) {
                    break;
                }
                outgoing_socket.write( buffer );
            } else {
                break; /* timeout */
            }
        }

    } catch ( const Exception & e ) {
        cerr.flush();
        e.perror();
        return;
    }

    cerr.flush();
    return;
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

int ferry( FileDescriptor & tun,
           FileDescriptor & sibling_fd,
           Socket & listen_socket_udp,
           const Address connect_addr_udp,
           Socket & listen_socket_tcp,
           const Address connect_addr_tcp,
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

    pollfds[ 3 ].fd = listen_socket_udp.raw_fd();
    pollfds[ 3 ].events = POLLIN;

    pollfds[ 4 ].fd = listen_socket_tcp.raw_fd();
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
            thread newthread( [&listen_socket_udp, &connect_addr_udp] ( const pair< Address, string > request ) {
                    service_udp_request( listen_socket_udp, request, connect_addr_udp ); },
                listen_socket_udp.recvfrom() );
            newthread.detach();
        }
        
        if ( pollfds[ 4 ].revents & POLLIN ) {
            /* got TCP dns request */
            thread newthread( [&connect_addr_tcp] ( Socket service_socket ) {
                    service_tcp_request( service_socket, connect_addr_tcp ); },
                listen_socket_tcp.accept() );
            newthread.detach();
        }

        /* packets FROM tail of delay queue go to sibling */
        delay_queue.write_packets( sibling_fd );
    }
}
