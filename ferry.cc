/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <poll.h>
#include <csignal>

#include "ferry.hh"
#include "ferry_queue.hh"
#include "signalfd.hh"
#include "poller.hh"

using namespace std;
using namespace PollerShortNames;

Result handle_signal( const signalfd_siginfo & sig,
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
            return Result( ResultType::Exit, child_process.exit_status() );
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

        return ResultType::Exit;
    default:
        throw Exception( "unknown signal" );
    }

    return ResultType::Continue;
}

int ferry( FileDescriptor & tun,
           FileDescriptor & sibling_fd,
           unique_ptr<DNSProxy> && dns_proxy,
           ChildProcess & child_process,
           const uint64_t delay_ms,
           unique_ptr<HTTPProxy> && http_proxy )
{
    /* Make the queue of datagrams */
    FerryQueue delay_queue( delay_ms );

    /* set up signal file descriptor */
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );

    Poller poller;

    /* tun device gets datagram -> read it -> give to delay_queue */
    poller.add_action( Poller::Action( tun, Direction::In,
                                       [&] () {
                                           delay_queue.read_packet( tun.read() );
                                           return ResultType::Continue;
                                       } ) );

    /* we get datagram from sibling process -> write it to tun device */
    poller.add_action( Poller::Action( sibling_fd, Direction::In,
                                       [&] () {
                                           tun.write( sibling_fd.read() );
                                           return ResultType::Continue;
                                       } ) );

    /* we get signal -> main screen turn on -> handle signal */
    poller.add_action( Poller::Action( signal_fd.fd(), Direction::In,
                                       [&] () {
                                           return handle_signal( signal_fd.read_signal(),
                                                                 child_process );
                                       } ) );

    /* add dns proxy TCP and UDP sockets */
    if ( dns_proxy ) {
        poller.add_action( Poller::Action( dns_proxy->udp_listener().fd(), Direction::In,
                                           [&] () {
                                               dns_proxy->handle_udp();
                                               return ResultType::Continue;
                                           } ) );

        poller.add_action( Poller::Action( dns_proxy->tcp_listener().fd(), Direction::In,
                                           [&] () {
                                               dns_proxy->handle_tcp();
                                               return ResultType::Continue;
                                           } ) );
    }

    if ( http_proxy ) {
        poller.add_action( Poller::Action( http_proxy->tcp_listener().fd(), Direction::In,
                                           [&] () {
                                               http_proxy->handle_tcp_get();
                                               return ResultType::Continue;
                                           } ) );
    }

    while ( true ) {
        int wait_time = delay_queue.wait_time();
        auto poll_result = poller.poll( wait_time );

        if ( poll_result.result == Poller::Result::Type::Exit ) {
            return poll_result.exit_status;
        }

        /* packets FROM tail of delay queue go to sibling */
        delay_queue.write_packets( sibling_fd );
    }
}
