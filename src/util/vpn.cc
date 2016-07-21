#include "vpn.hh"

#include <iostream>
#include <vector>

#include "config.h"

using namespace std;

VPN::VPN( const string & path_to_security_files, const Address & ingress) 
  : config_file_("/tmp/openvpn_config")
{
  vector<Address> nameservers;
  write_config_file(path_to_security_files, ingress, nameservers);
}

VPN::VPN( const string & path_to_security_files, const Address & ingress,
    const Address & nameserver ) 
  : config_file_("/tmp/openvpn_config")
{
  vector<Address> nameservers;
  nameservers.push_back(nameserver);
  write_config_file(path_to_security_files, ingress, nameservers);
}

VPN::VPN( const string & path_to_security_files, const Address & ingress,
    const vector<Address> & nameservers ) 
  : config_file_("/tmp/openvpn_config")
{
  write_config_file(path_to_security_files, ingress, nameservers);
}

VPN::~VPN() {

}

void VPN::write_config_file(const string & path_to_security_files, 
    const Address & ingress, 
    const vector<Address> & nameservers) {
  cout << "OpenVPN config file: " << config_file_.name() << endl;
  config_file_.write("port 1194\n");  // Just listen to the default port.
  config_file_.write("proto udp\n");  // On UDP
  config_file_.write("dev tun\n");    // On |tun| mode

  // Setup CA, certificate, and key.
  config_file_.write("ca " + path_to_security_files + "ca.crt\n");
  config_file_.write("cert " + path_to_security_files + "server.crt\n");
  config_file_.write("key " + path_to_security_files + "server.key\n");
  config_file_.write("dh " + path_to_security_files + "dh2048.pem\n");

  config_file_.write("server 10.8.0.0 255.255.255.0\n");

  // Allow outside world to connect to another subnet.
  config_file_.write("route " + ingress.str() + " 255.255.255.255\n");

  config_file_.write("push redirect-gateway def1 bypass-dhcp\n");

  for (auto nameserver : nameservers) {
    config_file_.write("dhcp-option DNS " + nameserver.ip() + "\n");
  }

  config_file_.write("keepalive 10 120\n");

  // Set user and group.
  config_file_.write("user nobody\n");
  config_file_.write("group nogroup\n");

  config_file_.write("com-lzo\n");
  config_file_.write("persist-key\n");
  config_file_.write("persist-tun\n");
  config_file_.write("status openvpn-status.log\n");
  config_file_.write("verb 3\n");
}

vector<string> VPN::start_command() {
  vector<string> result;
  result.push_back("sudo");
  result.push_back(OPENVPN);
  result.push_back("--config");
  result.push_back("/home/ubuntu/build/conf/openvpn.conf");
  return result;
}

