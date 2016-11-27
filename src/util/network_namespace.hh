/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef NETNAMESPACE_HH
#define NETNAMESPACE_HH

#include <string>

class NetworkNamespace
{
private:
    std::string name_;
    bool has_own_resolvconf_;
    bool has_entered_;

public:

    const std::string NETNS_DIR = "/var/run/netns";
    const std::string NETNS_ETC_DIR = "/etc/netns";

    NetworkNamespace( const std::string & name );
    ~NetworkNamespace();

    void create_resolvconf( const std::string & nameserver );
    void enter( void );
};

#endif
