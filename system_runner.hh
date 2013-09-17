#ifndef SYSTEM_RUNNER
#define SYSTEM_RUNNER

#include <string>
#include <cstdlib>

#include "exception.hh"

void run( const std::string & command )
{
  const int return_value = system( command.c_str() );
  std::string error_message = std::string( "system: \"" ) + command.c_str() + '"';
  if ( return_value < 0 ) {
    throw Exception( error_message );
  } else if ( return_value > 0 ) {
    throw Exception( error_message, "command returned "
		     + std::to_string( WEXITSTATUS( return_value ) ) );
  }
}

#endif /* SYSTEM_RUNNER */
