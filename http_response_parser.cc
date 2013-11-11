/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <assert.h>

#include "http_response_parser.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

void HTTPResponseParser::parse( const string & buf )
{
    /* append buf to internal buffer */
    internal_buffer_ += buf;

    /* determine if full request (headers and body) */
    while( true ) {
        /* headers finished, now determine if body is finished */
        if ( headers_finished_ ) {
            if ( body_left_ <= internal_buffer_.size() ) { /* body finished */
                /* Create HTTP request and add to complete_responses */
                body_.append( internal_buffer_.substr( 0, body_left_ ) ); /* store body */
                internal_buffer_.replace( 0, body_left_, string() );
                complete_responses_.emplace( HTTPResponse( headers_, status_line_, body_ ) );
                /* remove remaining part of current request from internal_buffer and reset */
                status_line_.erase();
                headers_.clear();
                body_.clear(); /* reset body */
                headers_finished_ = false;
                body_left_ = 0;
                return;
            } else { /* body_left > internal_buffer.size, so previous request body not complete */
                //body_left_ -= internal_buffer_.size();
                //internal_buffer_.erase();
                return;
            }
        }

        if ( headers_finished_ ) { /* headers finished but body not finished */
            return;
        }

        const string crlf = "\r\n";

        /* do we have a complete line? */
        size_t first_line_ending = internal_buffer_.find( crlf );
        if ( first_line_ending == std::string::npos ) {
        /* we don't have a full line yet */
            return;
        }
        /* we have a complete line */
        if ( status_line_.empty() ) { /* request line always comes first */
            status_line_ = internal_buffer_.substr( 0, first_line_ending );
        } else if ( first_line_ending == 0 ) { /* end of headers */
            if ( has_header( "Content-Length" ) ) {
                cout << "NOT CHUNKED" << endl;
            }
            headers_finished_ = true;
            body_left_ = body_len();
        } else { /* it's a header */
            headers_.emplace_back( internal_buffer_.substr( 0, first_line_ending ) );
        }

        /* delete parsed line from internal_buffer */
        internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
    }
}

bool HTTPResponseParser::has_header( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        if ( header.key() == header_name ) {
            return true;
        }
    }

    return false;
}

string HTTPResponseParser::get_header_value( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        if ( header.key() == header_name ) {
            return header.value();
        }
    }

    throw Exception( "HTTPHeaderParser header not found", header_name );
}

size_t HTTPResponseParser::get_chunk_size( void )
{
    /* call when internal buffer's first like is new chunk so it has size" */
    /* read internal buffer's first line which should have chunk size */
    string chunk_size;
    const string crlf = "\r\n";
    size_t end_of_line = internal_buffer_.find( crlf );
    chunk_size = internal_buffer_.substr( 0, end_of_line );
    return stol( chunk_size, nullptr, 16);
}

size_t HTTPResponseParser::body_len( void )
{
    assert( headers_parsed() );

    if ( has_header( "Content-Length" ) ) {
        return myatoi( get_header_value( "Content-Length" ) );
    } else if ( has_header( "Transfer-Encoding" ) ) {
        return get_chunk_size();
    }    
    throw Exception( "Cannot handle response format" );
}

HTTPResponse HTTPResponseParser::get_response( void )
{
    /* make sure there is a new complete response */
    assert( !empty() );

    HTTPResponse new_response;

    new_response = complete_responses_.front();

    complete_responses_.pop();

    return new_response;
}
