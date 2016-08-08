/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cstdlib>
#include <csignal>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>

#include "reverse_proxy.hh"
#include "system_runner.hh"
#include "config.h"
#include "util.hh"
#include "exception.hh"

using namespace std;

ReverseProxy::ReverseProxy( const Address & frontend_address, 
                            const Address & backend_address, 
                            const string & path_to_proxy,
                            const string & path_to_proxy_key,
                            const string & path_to_proxy_cert)
  : pidfile_("/tmp/replayshell_nghttpx.pid"),
    moved_away_(false)
{
    cout << "Frontend: " << frontend_address.str() << " backend: " << backend_address.str() << endl;
    cout << "Proxy key: " << path_to_proxy_key << " Proxy cert: " << path_to_proxy_cert << endl;
    stringstream frontend_arg;
    frontend_arg << "-f" << frontend_address.ip() << ","
                 << frontend_address.port();

    if (backend_address.port() == 443) {
      // Handle HTTPS
      stringstream https_backend_arg;
      https_backend_arg << "-b" << backend_address.ip() << ","
                        << backend_address.port() << ";/;tls";

      stringstream client_cert_arg;
      client_cert_arg << "--client-cert-file=" << string(MOD_SSL_CERTIFICATE_FILE);

      run( { path_to_proxy, frontend_arg.str(), https_backend_arg.str(),
          path_to_proxy_key, path_to_proxy_cert, client_cert_arg.str(),
          "-k", "--daemon", "--pid-file=" + pidfile_.name() } );
    } else {
      // Handle HTTP case
      stringstream http_backend_arg;
      http_backend_arg << "-b" << backend_address.ip() << ",80";
      run( { path_to_proxy, frontend_arg.str(), 
          http_backend_arg.str(), path_to_proxy_key, path_to_proxy_cert, 
          "--daemon", "--pid-file=" + pidfile_.name() } );
    }
}

ReverseProxy::ReverseProxy( const Address & frontend_address, 
                            const Address & backend_address, 
                            const string & path_to_proxy,
                            const string & path_to_proxy_key,
                            const string & path_to_proxy_cert,
                            const string & path_to_dependency_file)
  : pidfile_("/tmp/replayshell_nghttpx.pid"),
    moved_away_(false)
{
    cout << "Frontend: " << frontend_address.str() << " backend: " << backend_address.str() << endl;
    cout << "Proxy key: " << path_to_proxy_key << " Proxy cert: " << path_to_proxy_cert << endl;
    stringstream frontend_arg;
    frontend_arg << "-f" << frontend_address.ip() << ","
                 << frontend_address.port();

    if (backend_address.port() == 443) {
      // Handle HTTPS
      stringstream https_backend_arg;
      https_backend_arg << "-b" << backend_address.ip() << ","
                        << backend_address.port() << ";/;tls";
      stringstream client_cert_arg;
      client_cert_arg << "--client-cert-file=" << string(MOD_SSL_CERTIFICATE_FILE);

      run( { path_to_proxy, frontend_arg.str(), https_backend_arg.str(),
        path_to_proxy_key, path_to_proxy_cert, client_cert_arg.str(),
        "--dependency-filename", path_to_dependency_file, "--daemon",
        "-k", "--pid-file=" + pidfile_.name() } );
    } else {
      // Handle HTTP case
      stringstream http_backend_arg;
      http_backend_arg << "-b" << backend_address.ip() << ",80";
      run( { path_to_proxy, frontend_arg.str(), 
          http_backend_arg.str(), path_to_proxy_key, path_to_proxy_cert, 
          "--dependency-filename", path_to_dependency_file, "--daemon",
          "--pid-file=" + pidfile_.name() } );
    }
}

ReverseProxy::~ReverseProxy()
{
    if ( moved_away_ ) { return; }

    try {
      ifstream pidfile (pidfile_.name());
      string line;
      if (pidfile.is_open()) {
        getline(pidfile, line);
      }
      pidfile.close();
      int pid_int = atoi(line.c_str());
      kill(pid_int, SIGKILL);
    } catch ( const exception & e ) {
      print_exception( e );
    }
}

ReverseProxy::ReverseProxy( ReverseProxy && other )
    : pidfile_( move( other.pidfile_ ) ),
      moved_away_( false )
{
    other.moved_away_ = true;
}
