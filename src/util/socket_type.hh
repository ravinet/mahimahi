#ifndef SOCKET_TYPE_HH
#define SOCKET_TYPE_HH

#include <sys/types.h>

enum SocketType {
  UDP = SOCK_DGRAM,
  TCP = SOCK_STREAM
};

#endif /* SOCKET_TYPE_HH */
