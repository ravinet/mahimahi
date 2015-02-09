/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef EZIO_HH
#define EZIO_HH

#include <string>

namespace ezio
{
    const size_t read_chunk_size = 1 << 18;
}

std::string readall( const int fd, const size_t limit = ezio::read_chunk_size );
void writeall( const int fd, const std::string & buf );
std::string::const_iterator write_some( const int fd,
                                        const std::string::const_iterator & begin,
                                        const std::string::const_iterator & end );
long int myatoi( const std::string & str, const int base = 10 );

#endif /* EZIO_HH */
