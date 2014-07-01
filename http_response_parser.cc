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
