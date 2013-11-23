/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_RESPONSE_HH
#define HTTP_RESPONSE_HH

#include <vector>
#include <string>
#include <cassert>

#include "http_header.hh"
#include "body_parser.hh"

enum ResponseState { STATUS_LINE_PENDING, RESPONSE_HEADERS_PENDING, RESPONSE_BODY_PENDING, RESPONSE_COMPLETE };

class HTTPResponse
{
private:
    std::string status_line_;

    std::vector< HTTPHeader > headers_;

    std::string body_;

    ResponseState state_;

    size_t expected_body_size_;
    bool headers_specify_size_;

    void calculate_expected_body_size( void );

    std::string status_code( void ) const;

    bool request_was_head_;

    BodyType body_type_;

    std::string lower_case( const std::string & header_name ) const;

    std::string get_boundary( void ) const;

    std::string boundary_;

    BodyParser body_parser_;

    bool trailers_present_;

public:
    HTTPResponse( void )
        : status_line_(),
          headers_(),
          body_(),
          state_( STATUS_LINE_PENDING ),
          expected_body_size_(),
          headers_specify_size_(),
          request_was_head_( false ),
          body_type_(),
          boundary_(),
          body_parser_(),
          trailers_present_( false )
    {}

    void set_status_line( const std::string & s_status, const bool request_was_head );
    void add_header( const std::string & str );
    void done_with_headers( void );
    size_t read( const std::string & str );
    void done_with_body( void );

    size_t expected_body_size( void ) const { assert( state_ > RESPONSE_HEADERS_PENDING ); return expected_body_size_; }

    const ResponseState & state( void ) const { return state_; }

    std::string str( void ) const;

    void eof( void ) { state_ = RESPONSE_COMPLETE; }

    bool has_header( const std::string & header_name ) const;
    const std::string & get_header_value( const std::string & header_name ) const;

    bool body_size_is_known( void ) const { assert( state_ > RESPONSE_HEADERS_PENDING ); return headers_specify_size_; }
};

#endif /* HTTP_RESPONSE_HH */
