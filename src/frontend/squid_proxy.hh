#ifndef SQUID_PROXY_HH
#define SQUID_PROXY_HH

#include <string>
#include <vector>

#include "address.hh"
#include "temp_file.hh"

class SquidProxy {

private:
    /* each apache instance needs unique configuration file, error/access logs, and pid file */
    TempFile config_file_;
    bool moved_away_;

public:
    SquidProxy( const Address & addr );
    ~SquidProxy();

    SquidProxy( const SquidProxy & other ) = delete;
    SquidProxy & operator=( const SquidProxy & other ) = delete;

    SquidProxy( SquidProxy && other );
    SquidProxy & operator=( SquidProxy && other ) = delete;

    void run_squid( void );
    std::vector< std::string > command( void );
};

#endif // SQUID_PROXY_HH
