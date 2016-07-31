#include <iostream>
#include <fstream>
#include <sstream>

#include "pac_file.hh"

using namespace std;

PacFile::PacFile(const string & path) 
  : path_(path) 
{

}

void PacFile::WriteProxies(vector<pair<string, Address>> hostnames_to_addresses) {
  stringstream ss;
  ss << "function FindProxyForURL(url, host) {" << endl;
  
  for (auto it = hostnames_to_addresses.begin(); it != hostnames_to_addresses.end(); ++it) {
    auto hostname_to_address_pair = *it;
    auto hostname = hostname_to_address_pair.first;
    auto address = hostname_to_address_pair.second;
    ss << "if (shExpMatch(url, \"";
    ss << "*" << hostname_to_address_pair.first << "*";
    ss << "\"))";
    ss << "\treturn \"HTTPS " << address.str() << "\";" << endl;
  }

  ss << "}";
  cout << ss.str();
  ofstream output_file;
  output_file.open(path_);
  output_file << ss.str();
  output_file.close();
}

void PacFile::WriteProxies(
    vector<pair<string, Address>> hostnames_to_addresses,
    vector<pair<string, string>> hostnames_to_reverse_proxy_name) {
  stringstream ss;
  ss << "function FindProxyForURL(url, host) {" << endl;

  for (uint8_t i = 0; i < hostnames_to_addresses.size(); i++) {
  
  // for (auto it = hostnames_to_addresses.begin(); it != hostnames_to_addresses.end(); ++it) {
    auto hostname_to_address_pair = hostnames_to_addresses[i];
    auto hostname = hostname_to_address_pair.first;
    auto address = hostname_to_address_pair.second;
    auto hostname_to_reverse_server_name_pair = hostnames_to_reverse_proxy_name[i];
    ss << "if (shExpMatch(url, \"";
    ss << "*" << hostname_to_address_pair.first << "*";
    ss << "\"))";
    ss << "\treturn \"HTTPS " << hostname_to_reverse_server_name_pair.second << ":" << to_string(address.port()) << "\";" << endl;
  }

  ss << "}";
  cout << ss.str();
  ofstream output_file;
  output_file.open(path_);
  output_file << ss.str();
  output_file.close();

}
