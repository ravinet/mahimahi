/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_RESPONSE_PARSER_HH
#define HTTP_RESPONSE_PARSER_HH

#include <vector>
#include <string>
#include <queue>

#include "http_header.hh"
#include "http_response.hh"

class HTTPResponseParser
{
private:
    std::string internal_buffer_;

    std::string status_line_;

    std::vector< HTTPHeader > headers_;

    bool headers_finished_;

    std::string body_;

    size_t body_left_;

    std::queue< HTTPResponse > complete_responses_;

    bool chunked_;

    bool first_chunk_;

public:
    HTTPResponseParser() : internal_buffer_(),
		   status_line_(),
		   headers_(),
		   headers_finished_( false ),
                   body_(),
		   body_left_( 0 ),
                   complete_responses_(),
                   chunked_( false ),
                   first_chunk_( false )
    {}

    void parse( const std::string & buf );

    bool headers_parsed( void ) const { return headers_finished_; }

    std::string get_header_value( const std::string & header_name ) const;

    bool has_header( const std::string & header_name ) const;

    const std::string & status_line( void ) const { return status_line_; }

    bool empty( void ) const { return complete_responses_.empty(); }

    /* gets chunk size and appends chunk size line to body_ */
    size_t get_chunk_size( void );

    /* returns body size if response not chunked */
    size_t body_len( void );

    HTTPResponse get_response( void );

};

#endif /* HTTP_RESPONSE_PARSER_HH */
