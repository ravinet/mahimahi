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

#if 0
    if ( not is_chunked() ) {
        expected_body_size_ = calculate_expected_body_size();
    } else if ( is_chunked() ) { /* chunked */
        cout << "CHUNKED" << endl;
    } else { /* multipart */
        cout << "MULTIPART" << endl;
    }
#endif
}

void HTTPResponse::done_with_body( void )
{
    assert( state_ == RESPONSE_BODY_PENDING );

    state_ = RESPONSE_COMPLETE;

}

size_t HTTPResponse::read( const std::string & str )
{
    assert( state_ == RESPONSE_BODY_PENDING );

    size_t amount_parsed = body_parser_.read( str, body_type_, expected_body_size_ );

    if ( amount_parsed == std::string::npos ) { /* not enough to finish response */
        body_.append( str );
        return str.size();
    } else { /* finished response */
        body_.append( str.substr( 0, amount_parsed ) );
        state_ = RESPONSE_COMPLETE;
        return amount_parsed;
    }

    //return str.size(); /* XXX need to handle case where we don't read the whole thing */

#if 0
    if ( !is_chunked() ) {
        if ( body_.size() > expected_body_size_ ) {
            throw Exception( "HTTPResponse", "body was bigger than expected" );
        } else if ( body_.size() == expected_body_size_ ) {
            state_ = RESPONSE_COMPLETE;
        }
    }
#endif
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
        if ( header.key() == header_name ) {
            return true;
        }
    }

    return false;
}

const string & HTTPResponse::get_header_value( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        if ( header.key() == header_name ) {
            return header.value();
        }
    }

    throw Exception( "HTTPHeaderParser header not found", header_name );
}

string HTTPResponse::status_code( void ) const
{
    assert( state_ > STATUS_LINE_PENDING );
    auto tokens = split( status_line_, " " );
    if ( tokens.size() != 3 ) {
        throw Exception( "HTTPResponse", "Invalid status line" );
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
    } else {

        /* Rule 5 */
        headers_specify_size_ = false;
        body_type_ = IDENTITY_UNKNOWN;
    }

}
