/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CHUNKED_BODY_PARSER_HH
#define CHUNKED_BODY_PARSER_HH

#include <string>

class ChunkedBodyParser
{
private:
    std::string buffer_;

    std::string body_in_progress_;

    bool have_complete_line( void ) const;

    size_t chunk_size_;

    bool chunk_pending_;

    /* sets chunk size including CRLF after chunk */
    void get_chunk_size( void );

    /* returns true if it handled all trailers */
    bool handle_trailers( bool trailers );

public:
    ChunkedBodyParser( void )
        : buffer_(),
          body_in_progress_(),
          chunk_size_(),
          chunk_pending_( false )
    {}

    size_t parse( const std::string & str, bool trailers );
};

#endif /* CHUNKED_BODY_PARSER_HH */
