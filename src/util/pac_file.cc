#include <iostream>
#include <fstream>
#include <sstream>

#include "pac_file.hh"

using namespace std;

PacFile::PacFile(const string & path) 
  : path_(path) 
{

}

void PacFile::WriteDirect() {
  stringstream ss;
  ss << "function FindProxyForURL(url, host) {" << endl;
  ss << "\treturn \"DIRECT\";" << endl;
  ss << "}";
  ofstream output_file;
  output_file.open(path_);
  output_file << ss.str();
  output_file.close();
}

void PacFile::WriteProxies(vector<pair<string, Address>> hostnames_to_addresses) {
  stringstream ss;
  ss << "function FindProxyForURL(url, host) {" << endl;
  
  for (auto it = hostnames_to_addresses.begin(); it != hostnames_to_addresses.end(); ++it) {
    auto hostname_to_address_pair = *it;
    auto hostname = hostname_to_address_pair.first;
    auto address = hostname_to_address_pair.second;
    ss << "if (shExpMatch(url, \"";
    ss << ((address.port() == 80) ? "http:*" : "https:*");
    ss << hostname_to_address_pair.first << "*";
    ss << "\"))";
    if (address.port() == 80) {
      ss << "\treturn \"HTTPS " << address.str() << "\";" << endl;
    } else if (address.port() == 443) {
      // We don't need to connect via proxy if it is already a HTTPS request.
      ss << "\treturn \"DIRECT\";" << endl;
    }
  }

  ss << "}";
  ofstream output_file;
  output_file.open(path_);
  output_file << ss.str();
  output_file.close();
}

void PacFile::WriteProxies(vector<pair<string, Address>> hostnames_to_addresses,
                           string http_default_hostname,
                           Address http_default_address,
                           string https_default_hostname,
                           Address https_default_address) {
  stringstream ss;
  ss << "function FindProxyForURL(url, host) {" << endl;
  
  for (auto it = hostnames_to_addresses.begin(); it != hostnames_to_addresses.end(); ++it) {
    auto hostname_to_address_pair = *it;
    auto hostname = hostname_to_address_pair.first;
    auto address = hostname_to_address_pair.second;
    ss << "if (shExpMatch(url, \"";
    ss << ((address.port() == 80) ? "http:*" : "https:*");
    ss << hostname_to_address_pair.first << "*";
    ss << "\")) ";
    if (address.port() == 80) {
      ss << "return \"HTTPS " << address.str() << "\";" << endl;
    } else if (address.port() == 443) {
      // We don't need to connect via proxy if it is already a HTTPS request.
      ss << "return \"DIRECT\";" << endl;
    }
  }
  ss << "if (shExpMatch(url, \"https:*\")) ";
  ss << "\treturn \"DIRECT " << https_default_hostname << ":" << to_string(https_default_address.port()) << "\";" << endl;
  ss << "if (shExpMatch(url, \"http:*\")) ";
  ss << "\treturn \"HTTPS " << http_default_hostname << ":" << to_string(http_default_address.port()) << "\";" << endl;
  ss << "}";
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
    ss << ((address.port() == 80) ? "http:*" : "https:*");
    ss << hostname_to_address_pair.first << "*";
    ss << "\"))";
    if (address.port() == 80) {
      ss << "\treturn \"HTTPS " << hostname_to_reverse_server_name_pair.second << ":" << to_string(address.port()) << "\";" << endl;
    } else {
      ss << "\treturn \"DIRECT\";" << endl;
    }
  }

  ss << "}";
  ofstream output_file;
  output_file.open(path_);
  output_file << ss.str();
  output_file.close();
}

void PacFile::WriteProxies(
    vector<pair<string, Address>> hostnames_to_addresses,
    vector<pair<string, string>> hostnames_to_reverse_proxy_name,
    string default_hostname, Address default_address) {
  stringstream ss;
  ss << "function FindProxyForURL(url, host) {" << endl;

  for (uint8_t i = 0; i < hostnames_to_addresses.size(); i++) {
  
  // for (auto it = hostnames_to_addresses.begin(); it != hostnames_to_addresses.end(); ++it) {
    auto hostname_to_address_pair = hostnames_to_addresses[i];
    auto hostname = hostname_to_address_pair.first;
    auto address = hostname_to_address_pair.second;
    auto hostname_to_reverse_server_name_pair = hostnames_to_reverse_proxy_name[i];
    ss << "if (shExpMatch(url, \"";
    ss << ((address.port() == 80) ? "http:*" : "https:*");
    ss << hostname_to_address_pair.first << "*";
    ss << "\"))";
    if (address.port() == 80) {
      ss << "\treturn \"HTTPS " << hostname_to_reverse_server_name_pair.second << ":" << to_string(address.port()) << "\";" << endl;
    } else {
      ss << "\treturn \"DIRECT\";" << endl;
    }
  }
  ss << "\treturn \"HTTPS " << default_hostname << ":" << to_string(default_address.port()) << "\";" << endl;
  ss << "}";
  ofstream output_file;
  output_file.open(path_);
  output_file << ss.str();
  output_file.close();
}
