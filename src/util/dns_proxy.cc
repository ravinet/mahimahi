/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>

#include "dns_proxy.hh"
#include "poller.hh"
#include "bytestream_queue.hh"
#include "event_loop.hh"
#include "exception.hh"

using namespace std;
using namespace PollerShortNames;

template <typename SocketType>
SocketType make_bound_socket( const Address & listen_address )
{
    SocketType sock;
    sock.bind( listen_address );
    return sock;
}

DNSProxy::DNSProxy( const Address & listen_address, const Address & s_udp_target, const Address & s_tcp_target )
    : DNSProxy( make_bound_socket<UDPSocket>( listen_address ),
                make_bound_socket<TCPSocket>( listen_address ),
                s_udp_target, s_tcp_target )
{}

DNSProxy::DNSProxy( UDPSocket && udp_listener, TCPSocket && tcp_listener, const Address & s_udp_target, const Address & s_tcp_target )
    : udp_listener_( move( udp_listener ) ), tcp_listener_( move( tcp_listener ) ),
      udp_target_( s_udp_target ), tcp_target_( s_tcp_target )
{
    /* make sure the sockets are bound to something */
    if ( udp_listener_.local_address() == Address() ) {
        throw runtime_error( "DNSProxy internal error: udp_listener must be bound" );
    }

    if ( tcp_listener_.local_address() == Address() ) {
        throw runtime_error( "DNSProxy internal error: tcp_listener must be bound" );
    }

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
                UDPSocket dns_server;
                dns_server.connect( udp_target_ );
                dns_server.write( request.second );

                /* wait up to 60 seconds for a reply */
                Poller poller;

                poller.add_action( Poller::Action( dns_server, Direction::In,
                                                   [&] () {
                                                       udp_listener_.sendto( request.first,
                                                                             dns_server.read() );
                                                       return ResultType::Continue;
                                                   } ) );
                poller.poll( 60000 );
            } catch ( const exception & e ) {
                print_exception( e );
                return;
            }

            return;
        }, request );

    /* don't wait around for the reply */
    newthread.detach();
}

const static size_t BUFFER_SIZE = 1024 * 1024;

void DNSProxy::handle_tcp( void )
{
    /* start a new thread to handle request/reply */
    thread newthread( [&] ( Socket client ) {
            try {
                /* connect to DNS server */
                TCPSocket dns_server;
                dns_server.connect( tcp_target_ );

                Poller poller;

                /* Make bytestreams */
                ByteStreamQueue from_client( BUFFER_SIZE ), from_server( BUFFER_SIZE );

                poller.add_action( Poller::Action( dns_server, Direction::In,
                                                   [&] () { return eof( from_server.push( dns_server ) ) ? ResultType::Cancel : ResultType::Continue; },
                                                   from_server.space_available ) );

                poller.add_action( Poller::Action( client, Direction::In,
                                                   [&] () { return eof( from_client.push( client ) ) ? ResultType::Cancel : ResultType::Continue; },
                                                   from_client.space_available ) );

                poller.add_action( Poller::Action( dns_server, Direction::Out,
                                                   [&] () { from_client.pop( dns_server ); return ResultType::Continue; },
                                                   from_client.non_empty ) );

                poller.add_action( Poller::Action( client, Direction::Out,
                                                   [&] () { from_server.pop( client ); return ResultType::Continue; },
                                                   from_server.non_empty ) );

                while( true ) {
                    if ( poller.poll( 60000 ).result == Poller::Result::Type::Exit ) {
                        return;
                    }
                }
            } catch ( const exception & e ) {
                print_exception( e );
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
    } catch ( const exception & e ) {
        if ( string( e.what() ).substr( 0, 5 ) == "bind:" ) {
            return nullptr;
        } else {
            throw;
        }
    }
}

void DNSProxy::register_handlers( EventLoop & event_loop )
{
    event_loop.add_simple_input_handler( udp_listener(),
                                         [&] () { handle_udp(); return ResultType::Continue; } );
    event_loop.add_simple_input_handler( tcp_listener(),
                                         [&] () { handle_tcp(); return ResultType::Continue; } );
}
