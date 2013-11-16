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

    //bool full_;

public:
    /* constructor for full response or first chunked response */
/*    HTTPResponse( const std::vector< HTTPHeader > sheaders_, const std::string status, const std::string body )
        : status_line_( status ),
          headers_( sheaders_ ),
          body_( body ),
          state_( STATUS_LINE_PENDING ),
          expected_body_size_(),
          full_( true )
    {}*/

    /* default constructor to create temp in parser */
    HTTPResponse( void )
        : status_line_(),
          headers_(),
          body_(),
          state_( STATUS_LINE_PENDING ),
          expected_body_size_()
          //full_()
    {}

    /* constructor for intermediate/last chunk */
    /*HTTPResponse( const std::string body )
        : status_line_(),
          headers_(),
          body_( body ),
          expected_body_size_(),
          full_( false )
    {}*/

    void set_status_line( const std::string & s_status );
    void add_header( const std::string & str );
    void done_with_headers( void );
    void append_to_body( const std::string & str );

    size_t expected_body_size( void ) const { assert( state_ > RESPONSE_HEADERS_PENDING ); return expected_body_size_; }

    const ResponseState & state( void ) const { return state_; }

    std::string str( void ) const;

    bool has_header( const std::string & header_name ) const;
    const std::string & get_header_value( const std::string & header_name ) const;
};

#endif /* HTTP_RESPONSE_HH */
