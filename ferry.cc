/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <csignal>

#include "ferry.hh"
#include "util.hh"
#include "poller.hh"

using namespace std;
using namespace PollerShortNames;

template <class FerryType>
int packet_ferry( FerryType & ferry,
                  FileDescriptor & tun,
                  FileDescriptor & sibling_fd,
                  unique_ptr<DNSProxy> && dns_proxy,
                  ChildProcess && child_process )
{
    vector<ChildProcess> children;
    children.emplace_back( move( child_process ) );

    return packet_ferry( ferry, tun, sibling_fd, move( dns_proxy ), move( children ) );
}

template <class FerryType>
int packet_ferry( FerryType & ferry,
                  FileDescriptor & tun,
                  FileDescriptor & sibling_fd,
                  unique_ptr<DNSProxy> && dns_proxy,
                  vector<ChildProcess> && child_processes )
{
    /* set up signal file descriptor */
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );

    Poller poller;

    /* tun device gets datagram -> read it -> give to ferry */
    poller.add_action( Poller::Action( tun, Direction::In,
                                       [&] () {
                                           ferry.read_packet( tun.read() );
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
                                                                 child_processes );
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
        auto poll_result = poller.poll( ferry.wait_time() );

        if ( poll_result.result == Poller::Result::Type::Exit ) {
            return poll_result.exit_status;
        }

        /* packets FROM tail of ferry go to sibling */
        ferry.write_packets( sibling_fd );
    }
}
