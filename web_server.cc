/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <iostream>

#include "web_server.hh"
#include "apache_configuration.hh"
#include "system_runner.hh"
#include "config.h"
#include "util.hh"

using namespace std;

WebServer::WebServer( const string & listen_line, int port )
    : pid_file_name(),
      config_file( apache_main_config ),
      error_log( "" ),
      access_log( "" )
{
    /* make pid file using random number */
    pid_file_name = "/home/ravi/mahimahi/lock" + to_string( random() );

    /* if port 443, add ssl components */
    if ( port == 443 ) { /* ssl */
        config_file.append( apache_ssl_config );
    }

    /* add pid file, log files, and listen line to config file and run apache */
    config_file.append( "PidFile " + pid_file_name + "\n" );

    config_file.append( "ErrorLog /home/ravi/mahimahi/" + error_log.name() + "\n" );

    config_file.append( "CustomLog /home/ravi/mahimahi/" + access_log.name() + " common" + "\n" );

    config_file.append( listen_line );

    run( { APACHE2, "-f", "/home/ravi/mahimahi/" + config_file.name(), "-k", "start" } );
}

WebServer::~WebServer()
{
    run( { APACHE2, "-f", "/home/ravi/mahimahi/" + config_file.name(), "-k", "stop" } );

    /* delete pid file */
    SystemCall( "remove", remove( pid_file_name.c_str() ) );
}
