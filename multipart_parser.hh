/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef MULTIPART_BODY_PARSER_HH
#define MULTIPART_BODY_PARSER_HH

#include <string>

class MultipartBodyParser
{
private:
    std::string buffer_;

    std::string body_in_progress_;

    size_t part_size_;

    bool part_pending_;

    /* checks whether we have enough lines for complete part header (including boundary) */
    bool part_header_present( void );

    /* pops boundary, content-type, content-range, and empty line */
    void pop_part_header( void );

    /* returns size of part given Content-Range line */
    size_t get_part_size( const std::string & size_line );

    /* incomplete header from previous buffer */
    std::string previous_header_;

public:
    MultipartBodyParser( void )
        : buffer_(),
          body_in_progress_(),
          part_size_(),
          part_pending_( false ),
          previous_header_()
    {}

    size_t parse( const std::string & str, const std::string & boundary );
};

#endif /* MULTIPART_BODY_PARSER_HH */
