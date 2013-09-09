#include <time.h>

uint64_t timestamp( void )
{
  struct timespec ts;
  if ( clock_gettime( CLOCK_REALTIME, &ts ) < 0 ) {
    perror( "clock_gettime" );
    exit( 1 );
  }

  uint64_t millis = ts.tv_nsec / 1000000;
  millis += uint64_t( ts.tv_sec ) * 1000;

  return millis;
}

