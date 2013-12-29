/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef WEB_SERVER_HH
#define WEB_SERVER_HH

#include <string>

#include "temp_file.hh"

class WebServer
{
private:
    /* pid file cannot be created prior to running apache so we store name for deletion */
    std::string pid_file_name;

    /* each apache instance needs unique configuration file, error/access logs, and pid file */
    TempFile config_file;
    TempFile error_log;
    TempFile access_log;

public:
    WebServer( const std::string & listen_line, int port );
    ~WebServer();
};

#endif