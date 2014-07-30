#ifndef INT32_HH
#define INT32_HH

/* Helper class to represent a 32-bit integer that
   will be transmitted in network byte order */

#include <endian.h>
#include <string>

#include "exception.hh"

class Integer32 {
private:
  uint32_t contents_ = 0;

public:
  Integer32( void ) {}

  Integer32( const uint32_t contents ) : contents_( contents ) {}

  /* Construct integer from network-byte-order string */
  Integer32( const std::string & contents )
  {
    if ( contents.size() != sizeof( uint32_t ) ) {
      throw Exception( "int32 constructor", "size mismatch" );
    }

    contents_ = be32toh( *(uint32_t *)contents.data() );
  }

  /* Produce network-byte-order representation of integer */
  operator std::string () const
  {
    const uint32_t network_order = htobe32( contents_ );
    return std::string( (char *)&network_order, sizeof( network_order ) );
  }

  /* access underlying contents */
  operator const uint32_t & () const { return contents_; }
};

#endif
