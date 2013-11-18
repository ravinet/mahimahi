/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_RESPONSE_HH
#define HTTP_RESPONSE_HH

#include <vector>
#include <string>
#include <cassert>

#include "http_header.hh"

enum ResponseState { STATUS_LINE_PENDING, RESPONSE_HEADERS_PENDING, RESPONSE_BODY_PENDING, RESPONSE_COMPLETE };

class HTTPResponse
{
private:
    std::string status_line_;

    std::vector< HTTPHeader > headers_;

    std::string body_;

    ResponseState state_;

    size_t expected_body_size_;

    size_t calculate_expected_body_size( void ) const;

public:
    HTTPResponse( void )
        : status_line_(),
          headers_(),
          body_(),
          state_( STATUS_LINE_PENDING ),
          expected_body_size_()
    {}

    void set_status_line( const std::string & s_status );
    void add_header( const std::string & str );
    void done_with_headers( void );
    void append_to_body( const std::string & str );
    void done_with_body( void );

    size_t expected_body_size( void ) const { assert( state_ > RESPONSE_HEADERS_PENDING ); return expected_body_size_; }

    void get_chunk_size( std::string size_line );

    const ResponseState & state( void ) const { return state_; }

    std::string str( void ) const;

    bool is_chunked( void ) const;

    bool has_header( const std::string & header_name ) const;
    const std::string & get_header_value( const std::string & header_name ) const;
};

#endif /* HTTP_RESPONSE_HH */
