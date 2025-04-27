/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef NETNAMESPACE_HH
#define NETNAMESPACE_HH

#include <string>
#include <memory>

#include "temp_file.hh"

class NetworkNamespace
{

private:
    bool has_own_resolvconf_;
    std::unique_ptr<TempFile> resolvconf_file_;

    bool has_entered_;

public:

    const std::string NETNS_DIR = "/var/run/netns";
    const std::string NETNS_ETC_DIR = "/etc/netns";

    NetworkNamespace();
    ~NetworkNamespace();

    void create_resolvconf( const std::string & nameserver );
    void enter( void );
};

#endif
