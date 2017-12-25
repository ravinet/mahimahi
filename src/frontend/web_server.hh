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

    bool moved_away_;

public:
    WebServer( const Address & addr, const std::string & working_directory, const std::string & record_path );
    WebServer( const Address & addr, const std::string & working_directory, const std::string & record_path, const std::string & escaped_page );
    WebServer( const Address & addr, const std::string & working_directory, const std::string & record_path, const std::string & escaped_page, const std::string & dependency_file );
    // WebServer( const Address & addr, const std::string & working_directory, const std::string & record_path, const std::string & ssl_key, const std::string & ssl_cert );

    ~WebServer();

    /* ban copying */
    WebServer( const WebServer & other ) = delete;
    WebServer & operator=( const WebServer & other ) = delete;

    /* allow move constructor */
    WebServer( WebServer && other );

    /* ... but not move assignment operator */
    WebServer & operator=( WebServer && other ) = delete;
};

#endif
