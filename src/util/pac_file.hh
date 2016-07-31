#ifndef PAC_FILE_HH
#define PAC_FILE_HH

#include <vector>

#include "address.hh"

class PacFile
{
public:
  PacFile( const std::string & path );
  void WriteProxies(std::vector<std::pair<std::string, Address>> hostnames_to_addresses);
  void WriteProxies(
      std::vector<std::pair<std::string, Address>> hostnames_to_addresses,
      std::vector<std::pair<std::string, std::string>> hostnames_to_reverse_proxy_name);

private:
  std::string path_;

};

#endif /* PAC_FILE_HH */
