/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef WEB_SERVER_HH
#define WEB_SERVER_HH

#include <string>

#include "temp_file.hh"
#include "address.hh"

class WebServer
{
private:
    /* each apache instance needs unique configuration file, error/access logs, and pid file */
    TempFile config_file_;
    TempFile error_log_;
    TempFile access_log_;

    bool moved_away_;

public:
    WebServer( const Address & addr, const std::string & record_folder, const std::string & user );
    ~WebServer();

    WebServer( WebServer && other );
};

#endif
