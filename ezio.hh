/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef EZIO_HH
#define EZIO_HH

#include <string>

namespace ezio
{
    const size_t read_chunk_size = 1048576;
}

std::string readall( const int fd, const size_t limit = ezio::read_chunk_size );
void writeall( const int fd, const std::string & buf );
long int myatoi( const std::string & str );

#endif /* EZIO_HH */
