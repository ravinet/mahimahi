/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "ferry.hh"

using namespace std;
using namespace PollerShortNames;

template <class FerryQueueType>
int Ferry::loop( FerryQueueType & ferry_queue,
                 FileDescriptor & tun,
                 FileDescriptor & sibling_fd )
{
    /* set up ferry-specific actions */

    /* tun device gets datagram -> read it -> give to ferry */
    add_simple_input_handler( tun, 
                              [&] () {
                                  ferry_queue.read_packet( tun.read() );
                                  return ResultType::Continue;
                              } );

    /* we get datagram from sibling process -> write it to tun device */
    add_simple_input_handler( sibling_fd,
                              [&] () {
                                  tun.write( sibling_fd.read() );
                                  return ResultType::Continue;
                              } );

    /* ferry ready to write datagram -> send to sibling process */
    add_action( Poller::Action( sibling_fd, Direction::Out,
                                [&] () {
                                    ferry_queue.write_packets( sibling_fd );
                                    return ResultType::Continue;
                                },
                                [&] () { return ferry_queue.wait_time() <= 0; } ) );

    return internal_loop( [&] () { return ferry_queue.wait_time(); } );
}
