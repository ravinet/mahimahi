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
        if ( headers_finished_ && chunked_ ) { /* headers are finished and we know body is chunked */
            /* gets chunk size and appends chunk size line to body_ */
            if ( first_chunk_ ) { /* make response from headers, status line and first chunk body, set first_chunk to false */
                cout << "DEALS WITH FIRST CHUNK, body_left = " << body_left_ << " internal size = " << internal_buffer_.size() << endl;
                if ( body_left_ <= internal_buffer_.size() ) { /* we have first chunk */
                    /* store body and remove it from internal_buffer_*/
                    body_.append( internal_buffer_.substr( 0, body_left_ ) );
                    internal_buffer_.replace( 0, body_left_ + crlf.size(), string() );
                    /* SHOULD CHECK IF NEXT LINE IS CRLF or next chunk size (if CRLF, remove here and add in response str) */
                    /* create response and add to completed response queue */
                    complete_responses_.emplace( HTTPResponse( headers_, status_line_, body_, true ) );
                    body_.clear(); /* reset body */
                    first_chunk_ = false;
                    body_left_ = get_chunk_size();
                } else { /* chunk not in internal_buffer_ yet */
                    return;
                }
            } else { /* not first chunk so check if last chunk and make response from just body */
                if ( body_left_ == 0 ) { /* last chunk of size 0 */
                    /* add response for chunk and reset for next response */
                    cout << "DEALS WITH LAST CHUNK" << endl;
                    size_t crlf_loc;
                    while ( ( crlf_loc = internal_buffer_.find( crlf ) ) != 0 ) { /* optional footers present */
                        if ( crlf_loc == std::string::npos ) {
                            /* we don't have a full line yet */
                            return;
                        }
                        body_.append( internal_buffer_.substr(0, crlf_loc + crlf.size() ) );
                    }
                    body_.append( "\r\n" );
                    complete_responses_.emplace( HTTPResponse( body_  ) );
                    internal_buffer_.replace(0, body_.size(), string() );
                    status_line_.erase();
                    headers_.clear();
                    body_.clear(); /* reset body */
                    headers_finished_ = false;
                    body_left_ = 0;
                    chunked_ = false;
                } else { /* intermediate chunk */
                    cout << "DEALS WITH INTERMEDIATE CHUNK, body_left = " << body_left_ << " internal size = " << internal_buffer_.size() << endl;
                    if ( body_left_ <= internal_buffer_.size() ) { /* we have full chunk */
                        cout << "ACTUALLY DEALS WITH INTERMEDIATE" << endl;
                        /* store body and remove it from internal_buffer_ */
                        body_.append( internal_buffer_.substr( 0, body_left_ ) ); /* store body */
                        internal_buffer_.replace( 0, body_left_ + crlf.size(), string() );
                        /* create response and add to completed response queue */
                        complete_responses_.emplace( HTTPResponse( body_ ) );
                        body_.clear(); /* reset body */
                        body_left_ = get_chunk_size();
                    } else { /* chunk not in internal_buffer_ yet */
                        return;
                    }
                }
            }
        }

        /* headers finished and body not chunked, now determine if body is finished */
        if ( headers_finished_  && !chunked_) {
            cout << "DEALS WITH RESPONSE, body_left = " << body_left_ << " internal size = " << internal_buffer_.size() << endl;
            if ( body_left_ <= internal_buffer_.size() ) { /* body finished */
                /* Create HTTP response and add to complete_responses */
                body_.append( internal_buffer_.substr( 0, body_left_ ) ); /* store body */
                internal_buffer_.replace( 0, body_left_, string() );
                complete_responses_.emplace( HTTPResponse( headers_, status_line_, body_ ) );
                /* remove remaining part of current request from internal_buffer and reset */
                status_line_.erase();
                headers_.clear();
                body_.clear(); /* reset body */
                headers_finished_ = false;
                body_left_ = 0;
                cout << "FINISHED RESPONSE" << endl;
                if ( internal_buffer_.size() == 0 ) { return; }
            } else { /* body_left > internal_buffer.size, so previous request body not complete */
                return;
            }
        }

        if ( headers_finished_ ) { /* headers finished but body not finished */
            return;
        }

        /* do we have a complete line? */
        size_t first_line_ending = internal_buffer_.find( crlf );
        if ( first_line_ending == std::string::npos ) {
        /* we don't have a full line yet */
            return;
        }
        /* we have a complete line */
        if ( status_line_.empty() ) { /* request line always comes first */
            status_line_ = internal_buffer_.substr( 0, first_line_ending );
            /* delete parsed line from internal_buffer */
            internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
        } else if ( first_line_ending == 0 ) { /* end of headers */
            headers_finished_ = true;
            if ( has_header( "Content-Length" ) ) { /* not chunked so get body length */
                cout << "NOT CHUNKED" << endl;
                body_left_ = body_len();
                /* delete parsed line from internal_buffer */
                internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
            } else if ( has_header( "Transfer-Encoding" ) ) {
                if ( get_header_value( "Transfer-Encoding" ) == "chunked" ) {
                    cout << "CHUNKED" << endl;
                    chunked_ = true; /* chunked so don't get body length now */
                    first_chunk_ = true;
                    /* delete parsed line from internal_buffer */
                    internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
                    body_left_ = get_chunk_size(); 
                } else {
                    throw Exception( "Not valid repsonse format" );
                }
            } else {
                cout << "FAIL" << endl;
                throw Exception( "Not valid response format" );
            }
        } else { /* it's a header */
            //cout <<  internal_buffer_.substr( 0, first_line_ending ) << endl;
            headers_.emplace_back( internal_buffer_.substr( 0, first_line_ending ) );
            /* delete parsed line from internal_buffer */
            internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
        }
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
    assert( chunked_ );
    /* read internal buffer's first line which should have chunk size and add it to body_  */
    string chunk_size;
    size_t end_of_line = internal_buffer_.find( crlf );
    chunk_size = internal_buffer_.substr( 0, end_of_line );
    body_.append( internal_buffer_.substr(0, end_of_line + crlf.size() ) );
    internal_buffer_.replace( 0, end_of_line + crlf.size(), string() );
    return strtol( chunk_size.c_str(), nullptr, 16);
}

size_t HTTPResponseParser::body_len( void )
{
    assert( headers_parsed() );
    assert( !chunked_ );

    return myatoi( get_header_value( "Content-Length" ) );
}

HTTPResponse HTTPResponseParser::get_response( void )
{

    HTTPResponse new_response;

    new_response = complete_responses_.front();

    complete_responses_.pop();

    return new_response;
}
