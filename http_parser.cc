/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <assert.h>

#include "http_parser.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

/* parse a header line into a key and a value */
HTTPHeader::HTTPHeader( const string & buf )
  : key_(), value_()
{
    const string separator = ":";

    /* step 1: does buffer contain colon? */
    size_t colon_location = buf.find( separator );
    if ( colon_location == std::string::npos ) {
        fprintf( stderr, "Buffer: %s\n", buf.c_str() );
        throw Exception( "HTTPHeader", "buffer does not contain colon" ); 
    }

    /* step 2: split buffer */
    key_ = buf.substr( 0, colon_location );
    string value_temp = buf.substr( colon_location + separator.size() );

    /* strip whitespace */
    size_t first_nonspace = value_temp.find_first_not_of( " " );
    value_ = value_temp.substr( first_nonspace );

    /*
    fprintf( stderr, "Got header. key=[[%s]] value = [[%s]]\n",
             key_.c_str(), value_.c_str() );
    */
}

void HTTPParser::parse( const string & buf )
{
    /* append buf to internal buffer */
    internal_buffer_ += buf;

    /* determine if full request (headers and body) */
    while(1) {
        /* headers finished, now determine if body is finished */
        if ( headers_finished_ ) {
            if ( body_left_ <= internal_buffer_.size() ) { /* body finished */
                /* Create HTTP request and add to complete_requests */
                body_.append( internal_buffer_.substr( 0, body_left_ ) ); /* store body */
                internal_buffer_.replace( 0, body_left_, string() );
                complete_requests_.emplace( HTTPRequest( headers_, request_line_, body_ ) );

                /* remove remaining part of current request from internal_buffer and reset */
                request_line_.erase();
                headers_.clear();
                body_.clear(); /* reset body */
                headers_finished_ = false;
                body_left_ = 0;
                return;
            } else { /* body_left > internal_buffer.size, so previous request body not complete */
                body_left_ -= internal_buffer_.size();
                internal_buffer_.erase();
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
        if ( request_line_.empty() ) { /* request line always comes first */
            request_line_ = internal_buffer_.substr( 0, first_line_ending );
        } else if ( first_line_ending == 0 ) { /* end of headers */
            headers_finished_ = true;
            body_left_ = body_len();
        } else { /* it's a header */
            headers_.emplace_back( internal_buffer_.substr( 0, first_line_ending ) );
        }

        /* delete parsed line from internal_buffer */
        internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
    }
}

bool HTTPParser::has_header( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        if ( header.key() == header_name ) {
            return true;
        }
    }

    return false;
}

string HTTPParser::get_header_value( const string & header_name ) const
{
    for ( const auto & header : headers_ ) {
        if ( header.key() == header_name ) {
            return header.value();
        }
    }

    throw Exception( "HTTPHeaderParser header not found", header_name );
}

size_t HTTPParser::body_len( void ) const
{
    assert( headers_parsed() );
    if ( request_line_.substr( 0, 4 ) == "GET " ) {
        return 0;
    } else if ( request_line_.substr( 0, 5 ) == "POST " ) {
        if ( !has_header( "Content-Length" ) ) {
            throw Exception( "HTTPHeaderParser does not support chunked requests or lowercase headers", "sorry" );
        }

        //    fprintf( stderr, "CONTENT-LENGTH is %s\n", get_header_value( "Content-Length" ).c_str() );
        return myatoi( get_header_value( "Content-Length" ) );
    } else {
        throw Exception( "Cannot handle HTTP method", request_line_ );
    }
}

HTTPRequest HTTPParser::get_request( void )
{
    /* make sure there is a new complete request */
    assert( !empty() );

    HTTPRequest new_request;

    new_request = complete_requests_.front();

    complete_requests_.pop();

    return new_request;
}
