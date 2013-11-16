/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "http_request.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

void HTTPRequest::set_request_line( const std::string & s_request )
{
    assert( state_ == REQUEST_LINE_PENDING );
    request_line_ = s_request;
    state_ = HEADERS_PENDING;
}

void HTTPRequest::add_header( const std::string & str )
{
    assert( state_ == HEADERS_PENDING );
    headers_.emplace_back( str );
}

void HTTPRequest::done_with_headers( void )
{
    assert( state_ == HEADERS_PENDING );
    state_ = BODY_PENDING;

    expected_body_size_ = expected_body_size();
}

void HTTPRequest::append_to_body( const std::string & str )
{
    assert( state_ == BODY_PENDING );
    body_.append( str );

    if ( body_.size() > expected_body_size_ ) {
        throw Exception( "HTTPRequest", "body was bigger than expected" );
    } else if ( body_.size() == expected_body_size_ ) {
        state_ = COMPLETE;
    }
}

string HTTPRequest::str( void ) const
{
    assert( state_ == COMPLETE );

    const string CRLF = "\r\n";

    /* start with request line */
    string ret( request_line_ + CRLF );

    /* iterate through headers and add "key: value\r\n" to request */
    for ( const auto & header : headers_ ) {
        ret.append( header.str() + CRLF );
    }

    /* blank line between headers and body */
    ret.append( CRLF );

    /* add body to request */
    ret.append( body_ );

    return ret;
}

bool HTTPRequest::is_head( void ) const
{
    return request_line_.substr( 0, 5 ) == "HEAD ";
}

size_t HTTPRequest::calculate_expected_body_size( void ) const
{
    assert( state_ > HEADERS_PENDING );
    if ( request_line_.substr( 0, 4 ) == "GET "
         or request_line_.substr( 0, 5 ) == "HEAD " ) {
        return 0;
    } else if ( request_line_.substr( 0, 5 ) == "POST " ) {
        if ( !has_header( "Content-Length" ) ) {
            throw Exception( "HTTPHeaderParser", "does not support chunked requests or lowercase headers" );
        }

        return myatoi( get_header_value( "Content-Length" ) );
    } else {
        throw Exception( "Cannot handle HTTP method", request_line_ );
    }
}

bool HTTPRequest::has_header( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        if ( header.key() == header_name ) {
            return true;
        }
    }

    return false;
}

const string & HTTPRequest::get_header_value( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        if ( header.key() == header_name ) {
            return header.value();
        }
    }

    throw Exception( "HTTPHeaderParser header not found", header_name );
}
