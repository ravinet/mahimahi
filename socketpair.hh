#ifndef SOCKETPAIR_HH
#define SOCKETPAIR_HH

#include <utility>

static std::pair<FileDescriptor, FileDescriptor> make_pipe( void )
{
  int pipe[ 2 ];
  SystemCall( "socketpair", socketpair( AF_UNIX, SOCK_DGRAM, 0, pipe ) );
  return std::make_pair( pipe[ 0 ], pipe[ 1 ] );
}

#endif /* SOCKETPAIR_HH */
