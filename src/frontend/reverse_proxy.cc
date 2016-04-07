/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

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
                            const string & associated_domain,
                            const string & path_to_proxy,
                            const string & path_to_proxy_key,
                            const string & path_to_proxy_cert)
  : moved_away_(false)
{
    cout << "Proxy key: " << path_to_proxy_key << " Proxy cert: " << path_to_proxy_cert << " Associated domain: " << associated_domain << endl;
    stringstream frontend_arg;
    frontend_arg << "-f" << frontend_address.ip() << ","
                 << frontend_address.port() << ";no-tls";

    stringstream backend_arg;
    backend_arg << "-b" << backend_address.ip() << ","
                << backend_address.port();

    run( { path_to_proxy, frontend_arg.str(), backend_arg.str(), 
        path_to_proxy_key, path_to_proxy_cert, "--daemon" } );
}

ReverseProxy::~ReverseProxy()
{
    if ( moved_away_ ) { return; }

    try {
        run( { "pkill", "nghttpx" } );
    } catch ( const exception & e ) {
      print_exception( e );
    }
}

ReverseProxy::ReverseProxy( ReverseProxy && other )
    : moved_away_( false )
{
    other.moved_away_ = true;
}
