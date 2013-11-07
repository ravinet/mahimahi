/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <csignal>

#include "ferry.hh"
#include "ferry_queue.hh"
#include "util.hh"
#include "poller.hh"

using namespace std;
using namespace PollerShortNames;

int ferry_with_delay( FileDescriptor & tun,
                      FileDescriptor & sibling_fd,
                      unique_ptr<DNSProxy> && dns_proxy,
                      ChildProcess & child_process,
                      const uint64_t delay_ms )
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
