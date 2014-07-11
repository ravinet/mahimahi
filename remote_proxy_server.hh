/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef REMOTE_PROXY_SERVER_HH
#define REMOTE_PROXY_SERVER_HH

#include <string>

#include "temp_file.hh"
#include "address.hh"

class RemoteProxyServer
{
private:
    /* each apache instance needs unique configuration file, error/access logs, and pid file */
    TempFile config_file_;
    TempFile error_log_;
    TempFile access_log_;

    bool moved_away_;

public:
    RemoteProxyServer( const Address & addr, const std::string & user );
    ~RemoteProxyServer();

    /* ban copying */
    RemoteProxyServer( const RemoteProxyServer & other ) = delete;
    RemoteProxyServer & operator=( const RemoteProxyServer & other ) = delete;

    /* allow move constructor */
    RemoteProxyServer( RemoteProxyServer && other );

    /* ... but not move assignment operator */
    RemoteProxyServer & operator=( RemoteProxyServer && other ) = delete;
};

#endif
