/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "tokenize.hh"
#include "http_response.hh"
#include "exception.hh"
#include "ezio.hh"
#include "mime_type.hh"

#include "chunked_parser.hh"
#include "bulk_parser.hh"

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

        /* Rule 1: size known to be zero */
        set_expected_body_size( true, 0 );
    } else if ( has_header( "Transfer-Encoding" )
                and equivalent_strings( split( get_header_value( "Transfer-Encoding" ), "," ).back(),
                                        "chunked" ) ) {

        /* Rule 2: size dictated by chunked encoding */
        /* Rule 2 is a bit ambiguous, but we think section 3.6 makes this acceptable */

        set_expected_body_size( false );

        /* Create body_parser_ with trailers_enabled if requied (RFC 2616 section 14.40) */
        body_parser_ = unique_ptr< BodyParser >( new ChunkedBodyParser( has_header( "Trailer" ) ) );
    } else if ( (not has_header( "Transfer-Encoding" ) )
                and has_header( "Content-Length" ) ) {

        /* Rule 3: content-length header present to specify size */
        set_expected_body_size( true, myatoi( get_header_value( "Content-Length" ) ) );
    } else if ( has_header( "Content-Type" )
                and equivalent_strings( MIMEType( get_header_value( "Content-Type" ) ).type(),
                                        "multipart/byteranges" ) ) {

        /* Rule 4 */
        set_expected_body_size( false );
        throw Exception( "HTTPResponse", "unsupported multipart/byteranges without Content-Length" );
    } else if ( has_header( "Content-Type" )
                and equivalent_strings( get_header_value( "Content-Type" ), "application/x-bulkreply" ) ) {

        /* Bulk Response from Remote Proxy */
        set_expected_body_size( false );

        /* Create body_parser_ for bulk response */
        body_parser_ = unique_ptr< BodyParser >( new BulkBodyParser( ) );

        /* set response to bulk so we don't store it in completed responses */
        is_bulk_ = true;
    } else {
        /* Rule 5 */
        set_expected_body_size( false );

        body_parser_ = unique_ptr< BodyParser >( new Rule5BodyParser() );
    }
}

size_t HTTPResponse::read_in_body( const std::string & str, ByteStreamQueue & from_dest )
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
        return read_in_complex_body( str, from_dest );
    }
}

size_t HTTPResponse::read_in_complex_body( const std::string & str, ByteStreamQueue & from_dest )
{
    assert( state_ == BODY_PENDING );
    assert( body_parser_ );

    auto amount_parsed = body_parser_->read( str, from_dest );
    if ( amount_parsed == std::string::npos ) {
        /* all of it belongs to the body */
        body_.append( str );
        return str.size();
    } else {
        /* body is now complete */
        body_.append( str.substr( 0, amount_parsed ) );
        state_ = COMPLETE;
        return amount_parsed;
    }
}

bool HTTPResponse::eof_in_body( void )
{
    /* complex bodies sometimes allow an EOF to terminate the body */
    if ( body_parser_ ) {
        return body_parser_->eof();
    } else {
        throw Exception( "HTTPResponse", "got EOF in middle of body" );
    }
}

void HTTPResponse::set_request_was_head( void )
{
    assert( state_ == FIRST_LINE_PENDING );

    request_was_head_ = true;
}
