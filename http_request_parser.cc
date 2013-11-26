/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <cassert>

#include "http_request_parser.hh"
#include "exception.hh"
#include "http_request.hh"

using namespace std;

bool HTTPRequestParser::have_complete_line( void ) const
{
    size_t first_line_ending = internal_buffer_.find( CRLF );
    return first_line_ending != std::string::npos;
}

string HTTPRequestParser::pop_line( void )
{
    size_t first_line_ending = internal_buffer_.find( CRLF );
    assert( first_line_ending != std::string::npos );

    string first_line( internal_buffer_.substr( 0, first_line_ending ) );
    internal_buffer_.replace( 0, first_line_ending + CRLF.size(), string() );
    return first_line;
}

void HTTPRequestParser::parse( const string & buf )
{
    /* append buf to internal buffer */
    internal_buffer_ += buf;

    /* parse as much as we can */
    while ( parsing_step() ) {}
}

bool HTTPRequestParser::parsing_step( void ) {
    switch ( request_in_progress_.state() ) {
    case FIRST_LINE_PENDING:
        /* do we have a complete line? */
        if ( not have_complete_line() ) { return false; }

        /* got line, so add it to pending request */
        request_in_progress_.set_first_line( pop_line() );
        return true;
    case HEADERS_PENDING:
        /* do we have a complete line? */
        if ( not have_complete_line() ) { return false; }

        /* is line blank? */
        {
            string line( pop_line() );
            if ( line.empty() ) {
                request_in_progress_.done_with_headers();
            } else {
                request_in_progress_.add_header( line );
            }
        }
        return true;

    case BODY_PENDING:
        request_in_progress_.read_in_body( internal_buffer_.substr( 0, request_in_progress_.expected_body_size() ) );
        internal_buffer_.replace( 0, request_in_progress_.expected_body_size(), string() );
        return true;
    case COMPLETE:
        complete_requests_.push( request_in_progress_ );
        request_in_progress_ = HTTPRequest();
        return true;
    }

    assert( false );
    return false;
}
