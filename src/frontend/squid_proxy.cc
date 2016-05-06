#include <string>

#include "squid_proxy.hh"

#include "config.h"
#include "system_runner.hh"
#include "exception.hh"

using namespace std;

SquidProxy::SquidProxy( const string & squid_config_filename ) :
  config_filename_(squid_config_filename),
  moved_away_( false )
{
}

SquidProxy::~SquidProxy() {
    if ( moved_away_ ) { return; }

    try {
        run( { SQUID, "-f", config_filename_, "-k", "shutdown" } );
    } catch ( const exception & e ) { /* don't throw from destructor */
        print_exception( e );
    }
}

void SquidProxy::run_squid() {
  run( { SQUID, "-f", config_filename_ } );
}

std::vector< std::string > SquidProxy::command() {
  return { SQUID, "-f", config_filename_, "-NCd1" };
}
