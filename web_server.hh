/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef WEB_SERVER_HH
#define WEB_SERVER_HH

#include <string>

#include "temp_file.hh"

class WebServer
{
private:
    std::string config_file_name;
    std::string pid_file_name;

public:
    WebServer( const std::string & listen_line, int port );
    ~WebServer();
};

#endif
