/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_HEADER_HH
#define HTTP_HEADER_HH

#include <string>

class HTTPHeader
{
private:
    std::string key_, value_;

public:
    HTTPHeader( const std::string & buf );

    const std::string & key( void ) const { return key_; }
    const std::string & value( void ) const { return value_; }

    std::string str( void ) const { return key_ + ": " + value_; }
};

#endif /* HTTP_HEADER_HH */
