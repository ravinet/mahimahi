#ifndef PAC_FILE_HH
#define PAC_FILE_HH

#include <vector>

#include "address.hh"

class PacFile
{
public:
  PacFile( const std::string & path );
  void WriteProxies(std::vector<std::pair<std::string, Address>> hostnames_to_addresses);
  void WriteProxies(std::vector<std::pair<std::string, Address>> hostnames_to_addresses, 
                    std::string http_default_hostname,
                    Address http_default_address,
                    std::string https_default_hostname,
                    Address https_default_address);
  void WriteProxies(
      std::vector<std::pair<std::string, Address>> hostnames_to_addresses,
      std::vector<std::pair<std::string, std::string>> hostnames_to_reverse_proxy_name);
  void WriteProxies(
      std::vector<std::pair<std::string, Address>> hostnames_to_addresses,
      std::vector<std::pair<std::string, std::string>> hostnames_to_reverse_proxy_name, std::string default_hostname, Address default_address);
  void WriteDirect();

private:
  std::string path_;

};

#endif /* PAC_FILE_HH */
