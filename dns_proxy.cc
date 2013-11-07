/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>

#include "dns_proxy.hh"
#include "poller.hh"
#include "bytestream_queue.hh"

using namespace std;
using namespace PollerShortNames;

DNSProxy::DNSProxy( const Address & listen_address, const Address & s_udp_target, const Address & s_tcp_target )
    : udp_listener_( UDP ), tcp_listener_( TCP ),
      udp_target_( s_udp_target ), tcp_target_( s_tcp_target )
{
    udp_listener_.bind( listen_address );
    tcp_listener_.bind( listen_address );
    tcp_listener_.listen();
}

void DNSProxy::handle_udp( void )
{
    /* get a UDP request */
    pair< Address, string > request = udp_listener_.recvfrom();

    /* start a new thread to handle request/reply */
    thread newthread( [&] ( pair< Address, string > request ) {
            try {
                /* send request to the DNS server */
                Socket dns_server( UDP );
                dns_server.connect( udp_target_ );
                dns_server.write( request.second );

                /* wait up to 60 seconds for a reply */
                Poller poller;

                poller.add_action( Poller::Action( dns_server.fd(), Direction::In,
                                                   [&] () {
                                                       udp_listener_.sendto( request.first,
                                                                             dns_server.read() );
                                                       return ResultType::Continue;
                                                   } ) );
                poller.poll( 60000 );
            } catch ( const Exception & e ) {
                e.perror();
                return;
            }

            return;
        }, request );

    /* don't wait around for the reply */
    newthread.detach();
}

void DNSProxy::handle_tcp( void )
{
    /* start a new thread to handle request/reply */
    thread newthread( [&] ( Socket client ) {
            try {
                /* connect to DNS server */
                Socket dns_server( TCP );
                dns_server.connect( tcp_target_ );

                Poller poller;

                /* Make bytestreams */
                ByteStreamQueue from_client( ezio::read_chunk_size ), from_server( ezio::read_chunk_size );

                poller.add_action( Poller::Action( dns_server.fd(), Direction::In,
                                                   [&] () { return eof( from_server.push( dns_server.fd() ) ) ? ResultType::Cancel : ResultType::Continue; },
                                                   from_server.space_available ) );

                poller.add_action( Poller::Action( client.fd(), Direction::In,
                                                   [&] () { return eof( from_client.push( client.fd() ) ) ? ResultType::Cancel : ResultType::Continue; },
                                                   from_client.space_available ) );

                poller.add_action( Poller::Action( dns_server.fd(), Direction::Out,
                                                   [&] () { from_client.pop( dns_server.fd() ); return ResultType::Continue; },
                                                   from_client.non_empty ) );

                poller.add_action( Poller::Action( client.fd(), Direction::Out,
                                                   [&] () { from_server.pop( client.fd() ); return ResultType::Continue; },
                                                   from_server.non_empty ) );

                while( true ) {
                    if ( poller.poll( 60000 ).result == Poller::Result::Type::Exit ) {
                        return;
                    }
                }
            } catch ( const Exception & e ) {
                e.perror();
                return;
            }
            return;
        }, tcp_listener_.accept() );

    /* don't wait around for the reply */
    newthread.detach();
}

unique_ptr<DNSProxy> DNSProxy::maybe_proxy( const Address & listen_address, const Address & s_udp_target, const Address & s_tcp_target )
{
    try {
        return unique_ptr<DNSProxy>( new DNSProxy( listen_address, s_udp_target, s_tcp_target ) );
    } catch ( const Exception & e ) {
        if ( e.attempt() == "bind" ) {
            return nullptr;
        } else {
            throw;
        }
    }
}
