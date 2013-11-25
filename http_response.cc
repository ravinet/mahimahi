/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "http_response.hh"
#include "exception.hh"
#include "ezio.hh"
#include "tokenize.hh"
#include "mime_type.hh"

using namespace std;

void HTTPResponse::set_status_line( const std::string & s_response, const bool request_was_head )
{
    assert( state_ == STATUS_LINE_PENDING );
    status_line_ = s_response;
    state_ = RESPONSE_HEADERS_PENDING;
    request_was_head_ = request_was_head;
}

void HTTPResponse::add_header( const std::string & str )
{
    assert( state_ == RESPONSE_HEADERS_PENDING );
    headers_.emplace_back( str );
}

void HTTPResponse::done_with_headers( void )
{
    assert( state_ == RESPONSE_HEADERS_PENDING );
    state_ = RESPONSE_BODY_PENDING;

    calculate_expected_body_size();

}

void HTTPResponse::done_with_body( void )
{
    assert( state_ == RESPONSE_BODY_PENDING );

    state_ = RESPONSE_COMPLETE;

}

size_t HTTPResponse::read( std::string & str )
{
    assert( state_ == RESPONSE_BODY_PENDING );

    const string CRLF = "\r\n";

    if ( body_type_ == MULTIPART and first_read_ ) { /* store extra crlf's between headers and first boundary */
        while ( str.find( CRLF ) == 0 ) {
            body_.append( CRLF );
            str.replace( 0, CRLF.size(), string() );
        }
    }
    first_read_ = false;

    size_t amount_parsed = body_parser_.read( str, body_type_, expected_body_size_, boundary_, trailers_present_ );

    if ( amount_parsed == std::string::npos ) { /* not enough to finish response */
        body_.append( str );
        return str.size();
    } else { /* finished response */
        body_.append( str.substr( 0, amount_parsed ) );
        state_ = RESPONSE_COMPLETE;
        first_read_ = true;
        return amount_parsed;
    }
}

string HTTPResponse::str( void ) const
{
    assert( state_ == RESPONSE_COMPLETE );

    const string CRLF = "\r\n";

    /* start with status line */
    string ret( status_line_ + CRLF );

    /* iterate through headers and add "key: value\r\n" to response */
    for ( const auto & header : headers_ ) {
        ret.append( header.str() + CRLF );
    }
    ret.append( CRLF );

    /* add body to response */
    ret.append( body_ );

    return ret;
}

bool HTTPResponse::has_header( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        if ( header.key() == header_name or header.key() == lower_case( header_name ) ) {
            return true;
        }
    }

    return false;
}

const string & HTTPResponse::get_header_value( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        if ( lower_case( header.key() ) == lower_case( header_name ) ) {
            return header.value();
        }
    }

    throw Exception( "HTTPHeaderParser header not found", header_name );
}

string HTTPResponse::lower_case( const string & header_name ) const
{
    string upper_case = header_name;
    for ( size_t pos = 0; pos < header_name.length(); pos++ ) {
       upper_case[pos] = tolower(upper_case[pos] );
    }
    return upper_case;
}

string HTTPResponse::get_boundary( void ) const
{
    assert( state_ > STATUS_LINE_PENDING );
    assert( body_type_ == MULTIPART );

    string content_type = get_header_value( "Content-Type" ); /* must check if it can be Content-Type */
    size_t boundary_start = content_type.find( "boundary=" ) + 9;
    return ( string( "--" ) + content_type.substr( boundary_start, content_type.length() - boundary_start ) );
}

string HTTPResponse::status_code( void ) const
{
    assert( state_ > STATUS_LINE_PENDING );
    auto tokens = split( status_line_, " " );
    if ( tokens.size() < 3 ) {
        throw Exception( "HTTPResponse", "Invalid status line: " + status_line_ );
    }

    return tokens.at( 1 );
}

void HTTPResponse::calculate_expected_body_size( void )
{
    assert( state_ > RESPONSE_HEADERS_PENDING );

    /* implement rules of RFC 2616 section 4.4 ("Message Length") */

    if ( status_code().at( 0 ) == '1'
         or status_code() == "204"
         or status_code() == "304"
         or request_was_head_ ) {

        /* Rule 1 */

        headers_specify_size_ = true;
        expected_body_size_ = 0;
        body_type_ = IDENTITY_KNOWN;

    } else if ( has_header( "Transfer-Encoding" )
                and get_header_value( "Transfer-Encoding" ) != "identity" ) {

        /* Rule 2 */

        headers_specify_size_ = false;
        body_type_ = CHUNKED;
        if ( has_header( "Trailer" ) )  {
            trailers_present_ = true;
        }
    } else if ( (not has_header( "Transfer-Encoding" ) )
                and has_header( "Content-Length" ) ) {

        /* Rule 3 */

        headers_specify_size_ = true;
        body_type_ = IDENTITY_KNOWN;
        expected_body_size_ = myatoi( get_header_value( "Content-Length" ) );
    } else if ( has_header( "Content-Type" )
                and MIMEType( get_header_value( "Content-Type" ) ).type() == "multipart/byteranges" ) {

        /* Rule 4 */

        headers_specify_size_ = false;
        body_type_ = MULTIPART;
        boundary_ = get_boundary();
    } else {

        /* Rule 5 */
        headers_specify_size_ = false;
        body_type_ = IDENTITY_UNKNOWN;
    }

}
