/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "http_request.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

void HTTPRequest::calculate_expected_body_size( void )
{
    assert( state_ == BODY_PENDING );

    if ( first_line_.substr( 0, 4 ) == "GET "
         or first_line_.substr( 0, 5 ) == "HEAD " ) {
        set_expected_body_size( true, 0 );
    } else if ( first_line_.substr( 0, 5 ) == "POST " ) {
        if ( !has_header( "Content-Length" ) ) {
            throw Exception( "HTTPRequest", "does not support chunked requests" );
        }

        set_expected_body_size( true, myatoi( get_header_value( "Content-Length" ) ) );
    } else {
        throw Exception( "Cannot handle HTTP method", first_line_ );
    }
}

size_t HTTPRequest::read_in_complex_body( const std::string & )
{
    /* we don't support complex bodies */
    throw Exception( "HTTPRequest", "does not support chunked requests" );
}

bool HTTPRequest::eof_in_body( void )
{
    throw Exception( "HTTPRequest", "got EOF in middle of body" );
}

bool HTTPRequest::is_head( void ) const
{
    assert( state_ > FIRST_LINE_PENDING );
    /* RFC 2616 5.1.1 says "The method is case-sensitive." */
    return first_line_.substr( 0, 5 ) == "HEAD ";
}

size_t HTTPRequest::read_in_body( const std::string & str )
{
    assert( state_ == BODY_PENDING );

    if ( body_size_is_known() ) {
        /* body size known in advance */

        assert( body_.size() <= expected_body_size() );
        const size_t amount_to_append = min( expected_body_size() - body_.size(),
                                             str.size() );

        body_.append( str.substr( 0, amount_to_append ) );
        if ( body_.size() == expected_body_size() ) {
            state_ = COMPLETE;
        }

        return amount_to_append;
    } else {
        /* body size not known in advance */
        return read_in_complex_body( str );
    }
}
