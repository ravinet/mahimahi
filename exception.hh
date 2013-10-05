/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef EXCEPTION_HH
#define EXCEPTION_HH

#include <string>
#include <iostream>
#include <cstring>

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

    void perror( void ) const
    {
        std::cerr << attempt_ << ": " << error_ << std::endl << std::flush;
    }

    const std::string & attempt( void ) const { return attempt_; }
};

#endif
