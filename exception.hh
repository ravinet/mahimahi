#ifndef EXCEPTION_HH
#define EXCEPTION_HH

#include <string>
#include <iostream>
#include <unistd.h>
#include <string.h>

class Exception
{
private:
  std::string attempt_, error_;

public:
  Exception( const std::string s_attempt, const std::string s_error )
    : attempt_( s_attempt ), error_( s_error )
  {}

  Exception( const std::string s_attempt )
    : attempt_( s_attempt ), error_( strerror( errno ) )
  {}

  void die( void ) const
  {
    std::cerr << "Exception: " << attempt_ << ": " << error_ << std::endl;
    std::cerr << "Exiting on error.\n";
    exit( EXIT_FAILURE );
  }
};

#endif
