/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <poll.h>

#include <thread>

#include "dns_proxy.hh"

using namespace std;

DNSProxy::DNSProxy( const Address & listen_address, const Address & s_udp_target, const Address & s_tcp_target )
    : udp_listener_( SocketType::UDP ), tcp_listener_( SocketType::TCP ),
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
                Socket dns_server( SocketType::UDP );
                dns_server.connect( udp_target_ );
                dns_server.write( request.second );

                /* wait up to 60 seconds for a reply */
                struct pollfd pollfds[ 1 ];
                pollfds[ 0 ].fd = dns_server.raw_fd();
                pollfds[ 0 ].events = POLLIN;

                if ( poll( pollfds, 1, 60000 ) < 0 ) {
                    throw Exception( "poll" );
                }

                /* if we got a reply, send it back to client */
                if ( pollfds[ 0 ].revents & POLLIN ) {
                    udp_listener_.sendto( request.first, dns_server.read() );
                }
            } catch ( const Exception & e ) {
                cerr.flush();
                e.perror();
                return;
            }

            cerr.flush();
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
                Socket dns_server( SocketType::TCP );
                dns_server.connect( tcp_target_ );

                /* ferry bytes in both directions */
                struct pollfd pollfds[ 2 ];
                pollfds[ 0 ].fd = client.raw_fd();
                pollfds[ 0 ].events = POLLIN;
                pollfds[ 1 ].fd = dns_server.raw_fd();
                pollfds[ 1 ].events = POLLIN;

                while ( true ) {
                    if ( poll( pollfds, 2, 60000 ) < 0 ) {
                        throw Exception( "poll" );
                    }

                    /* ferry bytes in both directions */
                    if ( pollfds[ 0 ].revents & POLLIN ) {
                        string buffer = client.read();
                        if ( buffer.empty() ) { break; } /* EOF */
                        dns_server.write( buffer );
                    } else if ( pollfds[ 1 ].revents & POLLIN ) {
                        string buffer = dns_server.read();
                        if ( buffer.empty() ) { break; } /* EOF */
                        client.write( buffer );
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
