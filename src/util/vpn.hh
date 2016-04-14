/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef VPN_HH
#define VPN_HH

#include <string>
#include <vector>

#include "address.hh"
#include "temp_file.hh"

/* VPN */

class VPN
{
  private:
    TempFile config_file_;

  public:
    VPN( const std::string & path_to_security_files, const Address & ingress,
       const std::vector<Address> & nameserver );

    // Returns the start command for the VPN.
    std::vector<std::string> start_command();

    ~VPN();
};

#endif /* VPN_HH */
