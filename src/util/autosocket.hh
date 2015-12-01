#ifndef AUTOSOCKET_HH
#define AUTOSOCKET_HH

#include "socket.hh"

#include <iostream>
#include <tuple>

/* UDPSocket adapter that automatically
   replies to the source address of the most
   recently-received incoming datagram */

class AutoSocket : public UDPSocket
{
private:
  Address saved_address_ {};

public:
  using UDPSocket::UDPSocket;

  /* override read to store [authentic?] source address */
  std::string read( const size_t ) override
  {
    std::string ret;
    std::tie( saved_address_, ret ) = UDPSocket::recvfrom();
    std::cerr << "Saved reply address: " << saved_address_.str() << std::endl;
    return ret;
  }

  /* override write to send to the stored address */
  std::string::const_iterator write( const std::string & buffer, const bool ) override
  {
    if ( saved_address_ == Address() ) {
      std::cerr << "No reply address, so dropping datagram" << std::endl;
      register_write();
      return buffer.cbegin();
    }
    std::cerr << "sendto to " << saved_address_.str() << std::endl;

    UDPSocket::sendto( saved_address_, buffer );
    return buffer.cend();
  }
};

#endif /* AUTOSOCKET_HH */
