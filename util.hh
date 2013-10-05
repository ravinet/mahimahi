/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef UTIL_HH
#define UTIL_HH

#include <string>
#include <cstring>

#include "address.hh"

std::string shell_path( void );
void drop_privileges( void );
void check_requirements( const int argc, const char * const argv[] );
Address first_nameserver( void );
void prepend_shell_prefix( const uint64_t & delay_ms );
template <typename T> void zero( T & x ) { memset( &x, 0, sizeof( x ) ); }

#endif /* UTIL_HH */
