/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef LENGTH_VALUE_PARSER_HH
#define LENGTH_VALUE_PARSER_HH

#include <string>
#include <utility>

#include "int32.hh"

/* class to print an HTTP request/response to screen */

class LengthValueParser
{
private:
    Integer32 proto_size;
    std::string buffer;
   enum {SIZE, BODY} state_;
public:
    LengthValueParser()
        : proto_size(),
          buffer(),
          state_( SIZE )
    {};
    std::pair< bool, std::string> parse( const std::string & response );
};

#endif /* LENGTH_VALUE_PARSER_HH */
