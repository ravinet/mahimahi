/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef NAT_HH
#define NAT_HH

/* Network Address Translator */

#include "system_runner.hh"

class NAT
{
public:
  NAT()
  {
    run( "iptables -t nat -A POSTROUTING -j MASQUERADE" );
  }

  ~NAT()
  {
    run( "iptables -t nat -D POSTROUTING -j MASQUERADE" );
  }
};

#endif /* NAT_HH */
