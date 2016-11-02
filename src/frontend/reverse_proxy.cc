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
  : config_file_("/tmp/nghttpx_config.conf"),
    pidfile_("/tmp/replayshell_nghttpx.pid"),
    moved_away_(false)
{
    cout << "Frontend: " << frontend_address.str() << " backend: " << backend_address.str() << endl;
    cout << "Proxy key: " << path_to_proxy_key << " Proxy cert: " << path_to_proxy_cert << endl;

    config_file_.write("frontend=" + frontend_address.ip() + "," + 
        to_string(frontend_address.port()) + "\n");

    config_file_.write("private-key-file=" + path_to_proxy_key + "\n");
    config_file_.write("certificate-file=" + path_to_proxy_cert + "\n");

    config_file_.write("daemon=yes\n");
    config_file_.write("pid-file=" + pidfile_.name() + "\n");

    // stringstream frontend_arg;
    // frontend_arg << "-f" << frontend_address.ip() << ","
    //              << frontend_address.port();

    if (backend_address.port() == 443) {
      // Handle HTTPS
      config_file_.write("backend=" + backend_address.ip() + "," + 
          to_string(backend_address.port()) + ";/;tls\n");
      config_file_.write("insecure=yes\n");
      config_file_.write("client-cert-file=" + 
          string(MOD_SSL_CERTIFICATE_FILE) + "\n");

      // stringstream https_backend_arg;
      // https_backend_arg << "-b" << backend_address.ip() << ","
      //                   << backend_address.port() << ";/;tls";

      // stringstream client_cert_arg;
      // client_cert_arg << "--client-cert-file=" << string(MOD_SSL_CERTIFICATE_FILE);

      // run( { path_to_proxy, frontend_arg.str(), https_backend_arg.str(),
      //     path_to_proxy_key, path_to_proxy_cert, client_cert_arg.str(),
      //     "-k", "--daemon", "--pid-file=" + pidfile_.name() } );
    } else {
      // Handle HTTP case
      config_file_.write("backend=" + backend_address.ip() + ",80\n");
      // stringstream http_backend_arg;
      // http_backend_arg << "-b" << backend_address.ip() << ",80";
    }

    run( { path_to_proxy, "--conf=" + config_file_.name() } );
}

// ReverseProxy::ReverseProxy( const Address & frontend_address, 
//                             const Address & backend_address, 
//                             const string & path_to_proxy,
//                             const string & path_to_proxy_key,
//                             const string & path_to_proxy_cert,
//                             const string & path_to_dependency_file)
//   : config_file_("/tmp/nghttpx_config.conf"),
//     pidfile_("/tmp/replayshell_nghttpx.pid"),
//     moved_away_(false)
// {
//     cout << "Frontend: " << frontend_address.str() << " backend: " << backend_address.str() << endl;
//     cout << "Proxy key: " << path_to_proxy_key << " Proxy cert: " << path_to_proxy_cert << endl;
//     stringstream frontend_arg;
//     frontend_arg << "-f" << frontend_address.ip() << ","
//                  << frontend_address.port();
// 
//     if (backend_address.port() == 443) {
//       // Handle HTTPS
//       stringstream https_backend_arg;
//       https_backend_arg << "-b" << backend_address.ip() << ","
//                         << backend_address.port() << ";/;tls";
//       stringstream client_cert_arg;
//       client_cert_arg << "--client-cert-file=" << string(MOD_SSL_CERTIFICATE_FILE);
// 
//       run( { path_to_proxy, frontend_arg.str(), https_backend_arg.str(),
//         path_to_proxy_key, path_to_proxy_cert, client_cert_arg.str(),
//         "--dependency-filename", path_to_dependency_file, "--daemon",
//         "-k", "--pid-file=" + pidfile_.name() } );
//     } else {
//       // Handle HTTP case
//       stringstream http_backend_arg;
//       http_backend_arg << "-b" << backend_address.ip() << ",80";
//       run( { path_to_proxy, frontend_arg.str(), 
//           http_backend_arg.str(), path_to_proxy_key, path_to_proxy_cert, 
//           "--dependency-filename", path_to_dependency_file, "--daemon",
//           "--pid-file=" + pidfile_.name() } );
//     }
// }

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
    : config_file_( move( other.config_file_ ) ),
      pidfile_( move( other.pidfile_ ) ),
      moved_away_( false )
{
    other.moved_away_ = true;
}
