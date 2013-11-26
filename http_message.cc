/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "http_message.hh"
#include "exception.hh"

using namespace std;

/* methods called by an external parser */
void HTTPMessage::set_first_line( const string & str )
{
    assert( state_ == FIRST_LINE_PENDING );
    first_line_ = str;
    state_ = HEADERS_PENDING;
}

void HTTPMessage::add_header( const std::string & str )
{
    assert( state_ == HEADERS_PENDING );
    headers_.emplace_back( str );
}

void HTTPMessage::done_with_headers( void )
{
    assert( state_ == HEADERS_PENDING );
    state_ = BODY_PENDING;

    calculate_expected_body_size();
}

void HTTPMessage::set_expected_body_size( const bool is_known, const size_t value )
{
    assert( state_ == BODY_PENDING );
    
    expected_body_size_ = make_pair( is_known, value );
}

size_t HTTPMessage::read_in_body( const std::string & str )
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

void HTTPMessage::eof( void )
{
    switch ( state() ) {
    case FIRST_LINE_PENDING:
        return;
    case HEADERS_PENDING:
        throw Exception( "HTTPMessage", "EOF received in middle of headers" );
    case BODY_PENDING:
        return eof_in_body();
    case COMPLETE:
        assert( false ); /* nobody should be calling methods on a complete message */
        return;
    }
}

bool HTTPMessage::body_size_is_known( void ) const
{
    assert( state_ > HEADERS_PENDING );
    return expected_body_size_.first;
}

size_t HTTPMessage::expected_body_size( void ) const
{
    assert( body_size_is_known() );
    return expected_body_size_.second;
}

/* locale-insensitive ASCII conversion */
static char http_to_lower( char c )
{
    const char diff = 'A' - 'a';
    if ( c >= 'A' and c <= 'Z' ) {
        c -= diff;
    }
    return c;
}

/* check if two strings are equivalent per HTTP 1.1 comparison (case-insensitive) */
bool HTTPMessage::equivalent_strings( const string & a, const string & b )
{
    if ( a.size() != b.size() ) {
        return false;
    }

    for ( auto it_a = a.begin(), it_b = b.begin(); it_a < a.end(); it_a++, it_b++ ) {
        if ( http_to_lower( *it_a ) != http_to_lower( *it_b ) ) {
            return false;
        }
    }

    return true;
}

bool HTTPMessage::has_header( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        /* canonicalize header name per RFC 2616 section 2.1 */
        if ( equivalent_strings( header.key(), header_name ) ) {
            return true;
        }
    }

    return false;
}

const string & HTTPMessage::get_header_value( const std::string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        /* canonicalize header name per RFC 2616 section 2.1 */
        if ( equivalent_strings( header.key(), header_name ) ) {
            return header.value();
        }
    }

    throw Exception( "HTTPMessage header not found", header_name );
}

/* serialize the request or response as one string */
std::string HTTPMessage::str( void ) const
{
    assert( state_ == COMPLETE );

    /* start with first line */
    string ret( first_line_ + CRLF );

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
