/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef NAT_HH
#define NAT_HH

/* Network Address Translator */

#include <string>

#include "system_runner.hh"
#include "address.hh"

/* RAII class to make connections coming from the ingress address
   look like they're coming from the output device's address.

   We mark the connections on entry from the ingress address (with our PID),
   and then look for the mark on output. */

class NATRule {
private:
    std::vector< std::string > arguments;

public:
    NATRule( const std::vector< std::string > & s_args );
    ~NATRule();

    NATRule( const NATRule & other ) = delete;
    const NATRule & operator=( const NATRule & other ) = delete;
};

class NAT
{
private:
    NATRule pre_, post_;

public:
    NAT( const Address & ingress_addr );
};

class DNAT
{
private:
    NATRule rule_;

public:
    DNAT( const Address & listener, const std::string & interface );
};

#endif /* NAT_HH */
