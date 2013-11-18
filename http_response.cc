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
    }
    else { /* chunked */
        cout << "CHUNKED: ";
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
    /*if ( expected_body_size_ == 0 ) { // done with last chunk
        state_ = RESPONSE_COMPLETE;
    }*/
}

string HTTPResponse::str( void ) const
{
   assert( state_ == RESPONSE_COMPLETE );

    const string CRLF = "\r\n";

    string ret;

    /* check if response is a full response/first chunk or intermediate/last chunk */
        /* add request line to response */
        ret.append( status_line_ + CRLF );

        /* iterate through headers and add "key: value\r\n" to response */
        for ( const auto & header : headers_ ) {
            ret.append( header.str() + CRLF );
        }
        ret.append( CRLF );
   // }

    /* add body to response */
    ret.append( body_ );
    return ret;
}

void HTTPResponse::get_chunk_size( string size_line )
{
    assert( state_ > RESPONSE_HEADERS_PENDING );

    expected_body_size_ = strtol( size_line.c_str(), nullptr, 16 );
}

size_t HTTPResponse::calculate_expected_body_size( void ) const
{
    assert( state_ > RESPONSE_HEADERS_PENDING );
    
    if ( has_header( "Content-Length" ) ) { /* not chunked */
        return myatoi( get_header_value( "Content-Length" ) );
    } else { /* handle 304 Not Modified, 1xx, 204 No Content, and any response to HEAD request */
        return 0;
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
