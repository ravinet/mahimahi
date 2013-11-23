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

    bool part_header_present( void );

    void pop_part_header( void );

    size_t get_part_size( const std::string & size_line );


public:
    MultipartBodyParser( void )
        : buffer_(),
          body_in_progress_(),
          part_size_(),
          part_pending_( false )
    {}

    size_t parse( const std::string & str, const std::string & boundary );
};

#endif /* MULTIPART_BODY_PARSER_HH */
