/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BODY_PARSER_HH
#define BODY_PARSER_HH

#include <string>

#include "chunked_parser.hh"
#include "multipart_parser.hh"

enum BodyType { IDENTITY_KNOWN, IDENTITY_UNKNOWN, CHUNKED, MULTIPART };

class BodyParser
{
private:
    std::string buffer_;

    std::string body_in_progress_;

    bool have_complete_line( void ) const;

    ChunkedBodyParser chunked_parser;

    MultipartBodyParser multipart_parser;
public:
    BodyParser( void )
        : buffer_(),
          body_in_progress_(),
          chunked_parser(),
          multipart_parser()
    {}

    size_t read( const std::string & str, BodyType type, size_t expected_body_size, const std::string & boundary, bool trailers );
};

#endif /* BODY_PARSER_HH */
