/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_REQUEST_PARSER_HH
#define HTTP_REQUEST_PARSER_HH

#include <vector>
#include <string>
#include <queue>

#include "http_header.hh"
#include "http_request.hh"

class HTTPRequestParser
{
private:
    std::string internal_buffer_;

    HTTPRequest request_in_progress_;

    std::queue< HTTPRequest > complete_requests_;

    bool parsing_step( void );

    bool have_complete_line( void ) const;
    std::string pop_line( void );

public:
    HTTPRequestParser() : internal_buffer_(), request_in_progress_(), complete_requests_() {}

    void parse( const std::string & buf );

    bool empty( void ) const { return complete_requests_.empty(); }
    void pop( void ) { complete_requests_.pop(); }
    HTTPRequest & front( void ) { return complete_requests_.front(); }
};

#endif /* HTTP_REQUEST_PARSER_HH */
