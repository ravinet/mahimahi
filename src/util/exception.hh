/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef EXCEPTION_HH
#define EXCEPTION_HH

#include <system_error>
#include <iostream>
#include <typeinfo>

#include <cxxabi.h>

class tagged_error : public std::system_error
{
private:
    std::string attempt_and_error_;

public:
    tagged_error( const std::error_category & category,
                  const std::string & s_attempt,
                  const int error_code )
        : system_error( error_code, category ),
          attempt_and_error_( s_attempt + ": " + std::system_error::what() )
    {}

    const char * what( void ) const noexcept override
    {
        return attempt_and_error_.c_str();
    }
};

class unix_error : public tagged_error
{
public:
    unix_error( const std::string & s_attempt,
                const int s_errno = errno )
        : tagged_error( std::system_category(), s_attempt, s_errno )
    {}
};

inline void print_exception( const std::exception & e, std::ostream & output = std::cerr )
{
    output << "Died on " << abi::__cxa_demangle( typeid( e ).name(), nullptr, nullptr, nullptr ) << ": " << e.what() << std::endl;
}

/* error-checking wrapper for most syscalls */
inline int SystemCall( const std::string & s_attempt, const int return_value )
{
  if ( return_value >= 0 ) {
    return return_value;
  }

  throw unix_error( s_attempt );
}

#endif
