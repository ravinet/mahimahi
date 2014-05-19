/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <cassert>

#include "http_response_parser.hh"
#include "exception.hh"
#include "http_response.hh"

using namespace std;

void HTTPResponseParser::initialize_new_message( void )
{
    /* do we have a request that we expect to match this response up with? */
    if ( requests_were_head_.empty() ) {
        throw Exception( "HTTPResponseParser", "response without matching request" );
    }

    /* got line, so add it to pending request */
    if ( requests_were_head_.front() ) {
        message_in_progress_.set_request_was_head();
    }
    requests_were_head_.pop();
}

void HTTPResponseParser::new_request_arrived( const HTTPRequest & request )
{
    requests_were_head_.push( request.is_head() );
}

void HTTPResponseParser::parse( const std::string & buf, ByteStreamQueue & from_dest )
{
    if ( buf.empty() ) { /* EOF */
        message_in_progress_.eof();
    }

    /* append buf to internal buffer */
    buffer_.append( buf );

    /* parse as much as we can */
    while ( parsing_step( from_dest ) ) {}
}

bool HTTPResponseParser::parsing_step( ByteStreamQueue & from_dest )
{
    switch ( message_in_progress_.state() ) {
    case FIRST_LINE_PENDING:
        /* do we have a complete line? */
        if ( not buffer_.have_complete_line() ) { return false; }

        /* supply status line to request/response initialization routine */
        initialize_new_message();

        message_in_progress_.set_first_line( buffer_.get_and_pop_line() );

        return true;
    case HEADERS_PENDING:
        /* do we have a complete line? */
        if ( not buffer_.have_complete_line() ) { return false; }

        /* is line blank? */
        {
            std::string line( buffer_.get_and_pop_line() );
            if ( line.empty() ) {
                message_in_progress_.done_with_headers();
            } else {
                message_in_progress_.add_header( line );
            }
        }
        return true;

    case BODY_PENDING:
        {
            size_t bytes_read = message_in_progress_.read_in_body( buffer_.str(), from_dest );
            assert( bytes_read == buffer_.str().size() or message_in_progress_.state() == COMPLETE );
            buffer_.pop_bytes( bytes_read );
        }
        return message_in_progress_.state() == COMPLETE;

    case COMPLETE:
        if ( not message_in_progress_.is_bulk() ) { /* not a bulk response so we can store it */
            complete_messages_.emplace( std::move( message_in_progress_ ) );
        }
        message_in_progress_ = HTTPResponse();
        return true;
    }

    assert( false );
    return false;
}
