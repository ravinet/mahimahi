/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <unistd.h>
#include <cassert>

#include "ezio.hh"
#include "exception.hh"

using namespace std;

/* blocking write of entire buffer */
void writeall( const int fd, const string & buf )
{
    auto it = buf.begin();

    while ( it != buf.end() ) {
        it = write_some( fd, it, buf.end() );
    }
}

string::const_iterator write_some( const int fd,
                                   const string::const_iterator & begin,
                                   const string::const_iterator & end )
{
    assert( end > begin );

    ssize_t bytes_written = write( fd, &*begin, end - begin );

    if ( bytes_written < 0 ) {
        throw Exception( "write" );
    } else if ( bytes_written == 0 ) {
        throw Exception( "write returned 0" );
    }

    return begin + bytes_written;
}

string readall( const int fd, const size_t limit )
{
    char buffer[ ezio::read_chunk_size ];

    assert( limit > 0 );

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

long int myatoi( const string & str, const int base )
{
    if ( str.empty() ) {
        throw Exception( "Invalid integer string", "empty" );
    }

    char *end;

    errno = 0;
    long int ret = strtol( str.c_str(), &end, base );

    if ( errno != 0 ) {
        throw Exception( "strtol" );
    } else if ( end != str.c_str() + str.size() ) {
        throw Exception( "Invalid integer", str );
    }

    return ret;
}
