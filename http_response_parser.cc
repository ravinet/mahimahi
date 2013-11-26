/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <cassert>

#include "http_response_parser.hh"
#include "exception.hh"
#include "http_response.hh"

using namespace std;

static const string crlf = "\r\n";

bool HTTPResponseParser::have_complete_line( void ) const
{
    size_t first_line_ending = internal_buffer_.find( crlf );
    return first_line_ending != std::string::npos;
}

string HTTPResponseParser::pop_line( void )
{
    size_t first_line_ending = internal_buffer_.find( crlf );
    assert( first_line_ending != std::string::npos );

    string first_line( internal_buffer_.substr( 0, first_line_ending ) );
    internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
    return first_line;
}

void HTTPResponseParser::parse( const string & buf )
{
    if ( buf.empty() ) { /* EOF */
        response_in_progress_.eof();
    }        

    /* append buf to internal buffer */
    internal_buffer_ += buf;

    /* parse as much as we can */
    while ( parsing_step() ) {}
}

bool HTTPResponseParser::parsing_step( void ) {
    switch ( response_in_progress_.state() ) {
    case STATUS_LINE_PENDING:

        /* do we have a complete line? */
        if ( not have_complete_line() ) { return false; }

        /* do we have a request that we expect to match this response up with? */
        if ( requests_were_head_.empty() ) {
            throw Exception( "HTTPResponseParser", "response without matching request" );
        }

        /* got line, so add it to pending request */
        response_in_progress_.set_status_line( pop_line(), requests_were_head_.front() );
        requests_were_head_.pop();
        return true;
    case RESPONSE_HEADERS_PENDING:
        /* do we have a complete line? */
        if ( not have_complete_line() ) { return false; }

        /* is line blank? */
        {
            string line( pop_line() );
            if ( line.empty() ) {
                response_in_progress_.done_with_headers();
            } else {
                response_in_progress_.add_header( line );
            }
        }
        return true;

    case RESPONSE_BODY_PENDING:
        if ( internal_buffer_.empty() ) { return false; }

        {
            size_t bytes_read = response_in_progress_.read( internal_buffer_ );
            assert( bytes_read > 0 );
            internal_buffer_.replace( 0, bytes_read, string() );
        }
        return true;
    case RESPONSE_COMPLETE:
        complete_responses_.push( response_in_progress_ );
        response_in_progress_ = HTTPResponse();
        return true;
    }

    assert( false );
    return false;
}

void HTTPResponseParser::new_request_arrived( const HTTPRequest & request )
{
    requests_were_head_.push( request.is_head() );
}
