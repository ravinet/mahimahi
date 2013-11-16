/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_REQUEST_HH
#define HTTP_REQUEST_HH

#include <vector>
#include <string>
#include <cassert>

#include "http_header.hh"

enum RequestState { REQUEST_LINE_PENDING, HEADERS_PENDING, BODY_PENDING, COMPLETE };

class HTTPRequest
{
private:
    std::string request_line_;

    std::vector< HTTPHeader > headers_;

    std::string body_;

    RequestState state_;

    size_t expected_body_size_;

    size_t calculate_expected_body_size( void ) const;

public:
    HTTPRequest() : request_line_(), headers_(),
                    body_(), state_( REQUEST_LINE_PENDING ), expected_body_size_()
    {}

    void set_request_line( const std::string & s_request );
    void add_header( const std::string & str );
    void done_with_headers( void );
    void append_to_body( const std::string & str );

    size_t expected_body_size( void ) const { assert( state_ > HEADERS_PENDING ); return expected_body_size_; }

    const RequestState & state( void ) const { return state_; }

    std::string str( void ) const;

    bool has_header( const std::string & header_name ) const;
    const std::string & get_header_value( const std::string & header_name ) const;

    bool is_head( void ) const;
};

#endif /* HTTP_REQUEST_HH */
