/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef NAT_HH
#define NAT_HH

/* Network Address Translator */

#include "config.h"
#include "system_runner.hh"
#include "address.hh"

class NAT
{
private:
    class Rule {
    private:
        std::vector< std::string > arguments;

    public:
        Rule( const std::vector< std::string > & s_args )
            : arguments( s_args )
        {
            std::vector< std::string > command = { IPTABLES, "-t", "nat", "-A" };
            command.insert( command.end(), arguments.begin(), arguments.end() );
            run( command );
        }

        ~Rule()
        {
            std::vector< std::string > command = { IPTABLES, "-t", "nat", "-D" };
            command.insert( command.end(), arguments.begin(), arguments.end() );
            run( command );
        }

        Rule( const Rule & other ) = delete;
        const Rule & operator=( const Rule & other ) = delete;
    };

    Rule pre_, post_;

public:
    NAT( const Address & ingress_addr )
    : pre_( { "PREROUTING", "-s", ingress_addr.ip(), "-j", "CONNMARK", "--set-mark", "1" } ),
      post_( { "POSTROUTING", "-j", "MASQUERADE", "-m", "connmark", "--mark", "1" } )
    {}
};

#endif /* NAT_HH */
