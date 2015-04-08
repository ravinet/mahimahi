/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DNS_PROXY_HH
#define DNS_PROXY_HH

#include <memory>

#include "socket.hh"

class EventLoop;

class DNSProxy
{
private:
    UDPSocket udp_listener_;
    TCPSocket tcp_listener_;
    Address udp_target_, tcp_target_;

public:
    DNSProxy( const Address & listen_address, const Address & s_udp_target, const Address & s_tcp_target );

    UDPSocket & udp_listener( void ) { return udp_listener_; }
    TCPSocket & tcp_listener( void ) { return tcp_listener_; }

    void handle_udp( void );
    void handle_tcp( void );

    static std::unique_ptr<DNSProxy> maybe_proxy( const Address & listen_address, const Address & s_udp_target, const Address & s_tcp_target );

    void register_handlers( EventLoop & event_loop );
};

#endif /* DNS_PROXY_HH */
