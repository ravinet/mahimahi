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

/* read available bytes */
static const size_t read_chunk_size = 4096;

std::string readall( const int fd )
{
  char buffer[ read_chunk_size ];

  ssize_t bytes_read = read( fd, &buffer, read_chunk_size );

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
