/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DNS_SERVER_HH
#define DNS_SERVER_HH

#include <vector>
#include <string>

#include "child_process.hh"

ChildProcess start_dnsmasq( const std::vector< std::string > & extra_arguments );

#endif /* DNS_SERVER_HH */
