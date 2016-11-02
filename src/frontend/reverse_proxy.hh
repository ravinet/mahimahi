/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef REVERSE_PROXY_HH
#define REVERSE_PROXY_HH

#include <string>

#include "address.hh"
#include "temp_file.hh"

class ReverseProxy
{
private:
    TempFile config_file_;
    TempFile pidfile_;
    bool moved_away_;

public:
    ReverseProxy( const Address & frontend_address, 
                  const Address & backend_address, 
                  const std::string & path_to_proxy,
                  const std::string & path_to_proxy_key,
                  const std::string & path_to_proxy_cert);

    // ReverseProxy( const Address & frontend_address, 
    //               const Address & backend_address, 
    //               const std::string & path_to_proxy,
    //               const std::string & path_to_proxy_key,
    //               const std::string & path_to_proxy_cert,
    //               const std::string & path_to_dependency_file);

    ~ReverseProxy();

    /* ban copying */
    ReverseProxy( const ReverseProxy & other ) = delete;
    ReverseProxy & operator=( const ReverseProxy & other ) = delete;

    /* allow move constructor */
    ReverseProxy( ReverseProxy && other );

    /* ... but not move assignment operator */
    ReverseProxy & operator=( ReverseProxy && other ) = delete;
};

#endif // REVERSE_PROXY_HH
