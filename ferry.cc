/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// to run arping to send packet to ingress to/from machine with ip address addr  --> arping -I ingress addr

#include <poll.h>
#include <queue>
#include <iostream>

#include "ezio.hh"
#include "timestamp.hh"
#include "tapdevice.hh"
#include "exception.hh"

using namespace std;

int main ( void )
{
    // create two tap devices: ingress and egress
    TapDevice ingress_tap( "ingress" );
    TapDevice egress_tap( "egress" );

    // set up poll for tap devices
    struct pollfd pollfds[ 2 ];
    pollfds[ 0 ].fd = ingress_tap.fd();
    pollfds[ 0 ].events = POLLIN;

    pollfds[ 1 ].fd = egress_tap.fd();
    pollfds[ 1 ].events = POLLIN;

    // amount to delay each packet before forwarding
    uint64_t delay_ms = 2000;
    
    // queue of packets not yet forwarded
    queue< pair<uint64_t, string> > ingress_queue;
    queue< pair<uint64_t, string> > egress_queue;

    // poll on both tap devices
    while ( true ) {
       uint64_t now = timestamp();

       // set poll wait time for each queue to time before head packet must be sent
       uint64_t ingress_wait = ingress_queue.empty() ? UINT16_MAX : ingress_queue.front().first - now;
       uint64_t egress_wait = egress_queue.empty() ? UINT16_MAX : egress_queue.front().first - now;

       int wait_time = static_cast<int>(min(ingress_wait, egress_wait));

       if( poll( pollfds, 2, wait_time ) == -1 ) {
           throw Exception( "poll" );
       }

       now = timestamp();

       // read from ingress, which triggered POLLIN
       if ( pollfds[ 0 ].revents & POLLIN ) {
           ingress_queue.emplace( timestamp() + delay_ms, ingress_tap.read() );
       }

       // read from egress, which triggered POLLIN
       if ( pollfds[ 1 ].revents & POLLIN ) {
           egress_queue.emplace( timestamp() + delay_ms, egress_tap.read() );
       }

       // if packet in ingress queue and front ingress packet timestamp matches or is before current time, forward to egress
       if ( !ingress_queue.empty() && ingress_queue.front().first <= now ) {
           egress_tap.write( ingress_queue.front().second );
           ingress_queue.pop();
       }

       // if packet in egress queue and front egress packet timestamp matches or is before current time, forward to ingress
       if ( !egress_queue.empty() && egress_queue.front().first <= now ) {
           ingress_tap.write( egress_queue.front().second );
           egress_queue.pop();
       }
    }
}
