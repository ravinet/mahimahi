#ifndef EZIO_HH
#define EZIO_HH

#include <string>

std::string readall( const int fd );
void writeall( const int fd, const std::string & buf );

#endif /* EZIO_HH */
