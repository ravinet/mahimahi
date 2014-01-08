/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <iostream>

#include "web_server.hh"
#include "apache_configuration.hh"
#include "system_runner.hh"
#include "config.h"
#include "util.hh"

using namespace std;

WebServer::WebServer( const Address & addr, const string & record_folder, const string & user )
    : config_file_(),
      error_log_( "error" ),
      access_log_( "access" ),
      moved_away_( false )
{
    config_file_.write( apache_main_config + record_folder + "\n" );

    /* if port 443, add ssl components */
    if ( addr.port() == 443 ) { /* ssl */
        config_file_.write( apache_ssl_config );
    }

    /* add pid file, log files, user/group name, and listen line to config file and run apache */
    config_file_.write( "PidFile /tmp/replayshell.apache.pid." + to_string( random() ) + "\n" );
    /* Apache will check if this file exists before clobbering it,
       so we think it's ok for Apache to write here as root */

    config_file_.write( "ServerName mahimahi.\n" );

    config_file_.write( "ErrorLog " + error_log_.name() + "\n" );

    config_file_.write( "CustomLog " + access_log_.name() + " common" + "\n" );

    config_file_.write( "User " + user + "\n" );

    config_file_.write( "Group " + user + "\n" );

    config_file_.write( "Listen " + addr.ip() + ":" + to_string( addr.port() ) );

    run( { APACHE2, "-f", config_file_.name(), "-k", "start" } );
}

WebServer::~WebServer()
{
    if ( not moved_away_ ) {
        run( { APACHE2, "-f", config_file_.name(), "-k", "graceful-stop" } );
    }
}

WebServer::WebServer( WebServer && other )
    : config_file_( move( other.config_file_ ) ),
      error_log_( move( other.error_log_ ) ),
      access_log_( move( other.access_log_ ) ),
      moved_away_( false )
{
    other.moved_away_ = true;
}
