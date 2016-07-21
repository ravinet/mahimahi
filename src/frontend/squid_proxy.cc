#include <string>

#include "squid_proxy.hh"

#include "config.h"
#include "system_runner.hh"
#include "exception.hh"

using namespace std;

SquidProxy::SquidProxy( const Address & addr ) :
  config_file_( "/tmp/squid_config" ),
  moved_away_( false )
{
  std::string listening_port = std::to_string(addr.port());
  std::string path_prefix = PATH_PREFIX;
  config_file_.write("http_access allow all\n");
  config_file_.write("cache_effective_user vaspol\n");
  config_file_.write("http_port " + listening_port + " ssl-bump cert=" + path_prefix + "/certs/apple-pi.eecs.umich.edu.pem.crt key=" + path_prefix + "/certs/apple-pi.eecs.umich.edu.pk generate-host-certificates=on version=1 options=NO_SSLv2,NO_SSLv3,SINGLE_DH_USE\n");
  config_file_.write("sslproxy_cert_error allow all\n");
  config_file_.write("ssl_bump bump all\n");
  config_file_.write("coredump_dir " + path_prefix + "/var/cache/squid\n");
  config_file_.write("reply_header_access Cache-Control deny all\n");
  config_file_.write("reply_header_access Expires deny all\n");
  config_file_.write("reply_header_access Last-Modified deny all\n");
  config_file_.write("reply_header_access Date deny all\n");
  config_file_.write("reply_header_access Age deny all\n");
  config_file_.write("reply_header_access Etag deny all\n");
  config_file_.write("reply_header_replace Cache-Control max-age=3600\n");
  config_file_.write("refresh_pattern ^ftp:		1440	20%	10080\n");
  config_file_.write("refresh_pattern ^gopher:	1440	0%	1440\n");
  config_file_.write("refresh_pattern -i (/cgi-bin/|\?) 0	0%	0\n");
  config_file_.write("refresh_pattern .		0	20%	4320\n");
  config_file_.write("shutdown_lifetime 5 second\n");
}

SquidProxy::~SquidProxy() {
    if ( moved_away_ ) { return; }

    try {
        run( { SQUID, "-f", config_file_.name(), "-k", "shutdown" } );
    } catch ( const exception & e ) { /* don't throw from destructor */
        print_exception( e );
    }
}

void SquidProxy::run_squid() {
  run( { SQUID, "-f", config_file_.name() } );
}

std::vector< std::string > SquidProxy::command() {
  return { SQUID, "-f", config_file_.name(), "-NCd1" };
}
