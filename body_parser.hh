/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BODY_PARSER_HH
#define BODY_PARSER_HH

#include <string>

#include "http_response.hh"

class BodyParser
{
private:
    std::string buffer_;

    std::string body_in_progress_;

    bool have_complete_line( void ) const;

    size_t chunk_size_;

    bool chunk_pending_;

    void get_chunk_size( void );

    size_t part_size_;

    bool part_pending_;

    bool part_header_present( void );

    void pop_part_header( void );

    size_t get_part_size( const std::string & size_line );

    const std::string CRLF = "\r\n";    

public:
    BodyParser( void ) : buffer_(), body_in_progress_(), chunk_size_(), chunk_pending_( false ), part_size_(), part_pending_( false ) {}

    size_t read( const std::string & str, BodyType type, size_t expected_body_size, const std::string & boundary );
};

#endif /* BODY_PARSER_HH */
