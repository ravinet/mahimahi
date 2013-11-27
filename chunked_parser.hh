/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CHUNKED_BODY_PARSER_HH
#define CHUNKED_BODY_PARSER_HH

#include "body_parser.hh"

class ChunkedBodyParser : public BodyParser
{
public:
    std::string::size_type read( const std::string & ) override
    {
        throw Exception( "ChunkedBodyParser", "chunked body not yet supported" );
    }

    bool eof( void ) override
    {
        throw Exception( "ChunkedBodyParser", "invalid EOF in the middle of body" );
        return false;
    };
};

#endif /* CHUNKED_BODY_PARSER_HH */
