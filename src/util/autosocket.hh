#ifndef AUTOSOCKET_HH
#define AUTOSOCKET_HH

#include "socket.hh"

/* UDPSocket adapter that automatically
   replies to the source address of the most
   recently-received incoming datagram */

class AutoSocket : public UDPSocket
{
public:
  using UDPSocket::UDPSocket;
};

#endif /* AUTOSOCKET_HH */
