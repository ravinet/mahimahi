/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <unistd.h>

#include "ezio.hh"
#include "exception.hh"

using namespace std;

/* blocking write of entire buffer */
void writeall( const int fd, const string & buf )
{
    size_t total_bytes_written = 0;

    while ( total_bytes_written < buf.size() ) {
        ssize_t bytes_written = write( fd,
                                       buf.data() + total_bytes_written,
                                       buf.size() - total_bytes_written );

        if ( bytes_written < 0 ) {
            throw Exception( "write" );
        } else {
            total_bytes_written += bytes_written;
        }
    }
}

std::string readall( const int fd, const size_t limit )
{
    static char buffer[ ezio::read_chunk_size ];

    ssize_t bytes_read = read( fd, &buffer, min( ezio::read_chunk_size, limit ) );

    if ( bytes_read == 0 ) {
        /* end of file = client has closed their side of connection */
        return string();
    } else if ( bytes_read < 0 ) {
        throw Exception( "read" );
    } else {
        /* successful read */
        return string( buffer, bytes_read );
    }
}

long int myatoi( const string & str )
{
    if ( str.empty() ) {
        throw Exception( "Invalid integer string", "empty" );
    }

    char *end;

    errno = 0;
    long int ret = strtol( str.c_str(), &end, 10 );

    if ( errno != 0 ) {
        throw Exception( "strtol" );
    } else if ( end != str.c_str() + str.size() ) {
        throw Exception( "Invalid decimal integer", str );
    }

    return ret;
}
