#include <ctime>

#include "timestamp.hh"
#include "exception.hh"

uint64_t timestamp( void )
{
  struct timespec ts;
  if ( clock_gettime( CLOCK_REALTIME, &ts ) < 0 ) {
    throw Exception( "clock_gettime" );
  }

  uint64_t millis = ts.tv_nsec / 1000000;
  millis += uint64_t( ts.tv_sec ) * 1000;

  return millis;
}
