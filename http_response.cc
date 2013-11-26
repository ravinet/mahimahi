/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "tokenize.hh"
#include "http_response.hh"
#include "exception.hh"
#include "ezio.hh"
#include "mime_type.hh"

using namespace std;

string HTTPResponse::status_code( void ) const
{
    assert( state_ > FIRST_LINE_PENDING );
    auto tokens = split( first_line_, " " );
    if ( tokens.size() < 3 ) {
        throw Exception( "HTTPResponse", "Invalid status line: " + first_line_ );
    }

    return tokens.at( 1 );
}

void HTTPResponse::calculate_expected_body_size( void )
{
    assert( state_ == BODY_PENDING );

    /* implement rules of RFC 2616 section 4.4 ("Message Length") */

    if ( status_code().at( 0 ) == '1'
         or status_code() == "204"
         or status_code() == "304"
         or request_was_head_ ) {

        /* Rule 1 */

        set_expected_body_size( true, 0 );
        /* XXX set body type to identity, known */
    } else if ( has_header( "Transfer-Encoding" )
                and not equivalent_strings( get_header_value( "Transfer-Encoding" ),
                                            "identity" ) ) {

        /* Rule 2 */
        set_expected_body_size( false );

        /*
        body_type_ = CHUNKED;
        if ( has_header( "Trailer" ) )  {
            trailers_present_ = true;
        }
        */
    } else if ( (not has_header( "Transfer-Encoding" ) )
                and has_header( "Content-Length" ) ) {

        /* Rule 3 */
        set_expected_body_size( true, myatoi( get_header_value( "Content-Length" ) ) );
        /* XXX set body type to identity, known */
    } else if ( has_header( "Content-Type" )
                and equivalent_strings( MIMEType( get_header_value( "Content-Type" ) ).type(),
                                        "multipart/byteranges" ) ) {

        /* Rule 4 */
        set_expected_body_size( false );
        /* XXX set body type */
    } else {
        /* Rule 5 */
        set_expected_body_size( false );
        /* XXX set body type */
    }
}

size_t HTTPResponse::read_in_complex_body( const std::string & )
{
    /* we don't (yet) support complex bodies */
    /* XXX need to support */
    throw Exception( "HTTPResponse", "does not support chunked requests" );
}

void HTTPResponse::eof_in_body( void )
{
    /* XXX need to support */
    throw Exception( "HTTPResponse", "got EOF in middle of body" );
}

void HTTPResponse::set_request_was_head( void )
{
    assert( state_ == FIRST_LINE_PENDING );

    request_was_head_ = true;
}
