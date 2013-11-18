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

    HTTPResponse response_in_progress_;

    std::queue< HTTPResponse > complete_responses_;

    bool parsing_step( void );

    bool have_complete_line( void ) const;
    std::string pop_line( void );

    bool update_;

public:
    HTTPResponseParser() : internal_buffer_(), response_in_progress_(), complete_responses_(), update_( true ) {}

    void parse( const std::string & buf );

    bool empty( void ) const { return complete_responses_.empty(); }
    void pop( void ) { complete_responses_.pop(); }
    HTTPResponse & front( void ) { return complete_responses_.front(); }
};

#endif /* HTTP_RESPONSE_PARSER_HH */
