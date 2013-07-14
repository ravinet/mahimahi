#ifndef ADDRESS_HH
#define ADDRESS_HH

#include <netinet/in.h>
#include <string>

class Address
{
private:
  struct sockaddr_in addr_;

public:
  Address( const struct sockaddr_in &s_addr );
  Address( const std::string hostname, const std::string service );
  Address();

  std::string hostname( void ) const;
  uint16_t port( void ) const;
  std::string str( void ) const;

  const struct sockaddr_in & raw_sockaddr( void ) const { return addr_; }
};

#endif /* ADDRESS_HH */
