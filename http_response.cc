/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "http_response.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

void HTTPResponse::set_status_line( const std::string & s_response )
{
    assert( state_ == STATUS_LINE_PENDING );
    status_line_ = s_response;
    state_ = RESPONSE_HEADERS_PENDING;
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

    if ( not is_chunked() ) {
        expected_body_size_ = calculate_expected_body_size();
    } else if ( is_chunked() ) { /* chunked */
        cout << "CHUNKED" << endl;
    } else { /* multipart */
        cout << "MULTIPART" << endl;
    }
}

void HTTPResponse::done_with_body( void )
{
    assert( state_ == RESPONSE_BODY_PENDING );

    state_ = RESPONSE_COMPLETE;

}
void HTTPResponse::append_to_body( const std::string & str )
{
    assert( state_ == RESPONSE_BODY_PENDING );
    body_.append( str );
    if ( !is_chunked() ) {
        if ( body_.size() > expected_body_size_ ) {
            throw Exception( "HTTPResponse", "body was bigger than expected" );
        } else if ( body_.size() == expected_body_size_ ) {
            state_ = RESPONSE_COMPLETE;
        }
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

void HTTPResponse::get_chunk_size( const string & size_line )
{
    assert( state_ > RESPONSE_HEADERS_PENDING );

    expected_body_size_ = strtol( size_line.c_str(), nullptr, 16 );
}

void HTTPResponse::get_part_size( const string & size_line )
{
    assert( state_ > RESPONSE_HEADERS_PENDING );
    assert( is_multipart() );

    /* Content-range: bytes 500-899/8000 */
    size_t dash = size_line.find( "-" );
    size_t range_start = size_line.find( "bytes" ) + 6;
    size_t range_end = size_line.find( "/" ) - 1;

    string lower_bound = size_line.substr( range_start, dash - range_start );
    string upper_bound = size_line.substr( dash + 1, range_end - dash );

    expected_body_size_ = strtol(upper_bound.c_str(), nullptr, 16 ) - strtol( lower_bound.c_str(), nullptr, 16 );
}

string HTTPResponse::get_boundary( void ) const
{
    assert( state_ > STATUS_LINE_PENDING );
    assert( is_multipart() );

    string content_type = get_header_value( "Content-type" );
    size_t boundary_start = content_type.find( "boundary=" ) + 9;
    return ( string( "--" ) + content_type.substr( boundary_start, content_type.length() - boundary_start ) );
}

size_t HTTPResponse::calculate_expected_body_size( void ) const
{
    assert( state_ > RESPONSE_HEADERS_PENDING );
    assert( !is_chunked() );
    string status_code = status_line_.substr( status_line_.find( " " ) + 1, 3 );
    if ( status_code.substr( 0, 1 ) == "1" or status_code == "204" or status_code == "304" ) { /* body size is 0 if 1xx, 204, or 304 */
        return 0;
    } else if ( has_header( "Content-Length" ) ) { /* not chunked: body size is value of Content-Length" */
        return myatoi( get_header_value( "Content-Length" ) );
    } else { /* Response to Head request */
        throw Exception( "NOT HANDLED: ", status_line_ );
    }
}

bool HTTPResponse::is_chunked( void ) const
{
    assert( state_ > STATUS_LINE_PENDING );
    if ( has_header( "Transfer-Encoding" ) ) {
        if ( get_header_value( "Transfer-Encoding" ) == "chunked" ) {
            return true;
        }
        else {
            throw Exception( "Transfer-Encoding: " + get_header_value( "Transfer-Encoding" ), "Not valid encoding format" );
        }
    }
    return false;
}

bool HTTPResponse::is_multipart( void ) const
{
    assert( state_ > STATUS_LINE_PENDING );
    if ( has_header( "Content-type" ) ) {
        if ( get_header_value( "Content-type" ).find( "multipart/byteranges" ) != std::string::npos ) {
            return true;
        }
    }
    return false;

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
