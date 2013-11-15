/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <assert.h>

#include "http_response_parser.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

void HTTPResponseParser::parse( const string & buf )
{
    internal_buffer_ += buf;
    while( true ) {
        if ( headers_finished_ && chunked_ ) { /* headers are finished and we know body is chunked */
            if ( update_ ) { /* update body_left_ to handle next chunk */
                if ( internal_buffer_.find( crlf ) == std::string::npos ) { /* no line for next chunk size yet */
                    return;
                }
                body_left_ = get_chunk_size();
            }
            if ( first_chunk_ ) { /* update buffer, make response, and set first_chunk to false */
                if ( body_left_ <= internal_buffer_.size() ) { /* we have first chunk */
                    body_.append( internal_buffer_.substr( 0, body_left_ + crlf.size() ) );
                    internal_buffer_.replace( 0, body_left_ + crlf.size(), string() );
                    complete_responses_.emplace( HTTPResponse( headers_, status_line_, body_ ) );
                    body_.clear();
                    first_chunk_ = false;
                    update_ = true;
                } else { /* we don't have first chunk yet */
                    update_ = false;
                    return;
                }
            } else { /* intermediate or last chunk, update buffer and make response (if last chunk, reset) */
                if ( body_left_ == 0 ) { /* last chunk of size 0 */
                    size_t crlf_loc;
                    if ( has_header( "Trailer" ) ) { /* trailers present */
                        while ( ( crlf_loc = internal_buffer_.find( crlf ) ) != 0 ) {
                            if ( crlf_loc == std::string::npos ) {
                                /* we don't have a full line for trailer */
                                return;
                            }
                            body_.append( internal_buffer_.substr( 0, crlf_loc + crlf.size() ) );
                            internal_buffer_.replace( 0, crlf_loc + crlf.size(), string() );
                        }
                    }
                    body_.append( crlf );
                    complete_responses_.emplace( HTTPResponse( body_  ) );
                    internal_buffer_.replace( 0, crlf.size(), string() );
                    status_line_.erase();
                    headers_.clear();
                    body_.clear();
                    headers_finished_ = false;
                    body_left_ = 0;
                    chunked_ = false;
                    update_ = true;
                } else { /* intermediate chunk */
                    if ( body_left_ <= internal_buffer_.size() ) { /* we have full chunk */
                        body_.append( internal_buffer_.substr( 0, body_left_ + crlf.size() ) );
                        internal_buffer_.replace( 0, body_left_ + crlf.size(), string() );
                        complete_responses_.emplace( HTTPResponse( body_ ) );
                        body_.clear();
                        update_ = true;
                    } else { /* we don't have chunk yet */
                        update_ = false;
                        return;
                    }
                }
            }
        }
        /* headers finished and body not chunked */
        if ( headers_finished_  && !chunked_ ) {
            if ( body_left_ <= internal_buffer_.size() ) { /* we have body so make response and reset */
                body_.append( internal_buffer_.substr( 0, body_left_ + crlf.size() ) );
                internal_buffer_.replace( 0, body_left_ + crlf.size(), string() );
                complete_responses_.emplace( HTTPResponse( headers_, status_line_, body_ ) );
                status_line_.erase();
                headers_.clear();
                body_.clear();
                headers_finished_ = false;
                body_left_ = 0;
            } else { /* we don't have full body yet */
                return;
            }
        }


        /* headers not finished */
        if ( !headers_finished_ ) {
            /* do we have a complete line? */
            size_t first_line_ending = internal_buffer_.find( crlf );
            if ( first_line_ending == std::string::npos ) {
                /* we don't have a full line yet */
                return;
            }
            /* we have a complete line */
            if ( status_line_.empty() ) { /* request line always comes first */
                status_line_ = internal_buffer_.substr( 0, first_line_ending );
                internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
            } else if ( first_line_ending == 0 ) { /* end of headers */
                headers_finished_ = true;
                if ( has_header( "Transfer-Encoding" ) ) {
                    if ( get_header_value( "Transfer-Encoding" ) == "chunked" ) {
                        cout << "CHUNKED" << endl;
                        chunked_ = true; /* chunked */
                        first_chunk_ = true;
                        internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
                    } else {
                        throw Exception( "Transfer-Encoding", "Not valid encoding format" );
                    }
                } else if ( has_header( "Content-Length" ) ) { /* not chunked */
                    body_left_ = body_len();
                    internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
                } else { /* handle 304 Not Modified, 1xx, 204 No Content, and any response to HEAD request */
                    body_left_ = 0;
                }
            } else { /* it's a header */
                headers_.emplace_back( internal_buffer_.substr( 0, first_line_ending ) );
                internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
            }
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
    assert( headers_parsed() );
    assert( chunked_ );

    /* read internal buffer's first line, get chunk size, and add chunk size line to body_ */
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
    assert( !complete_responses_.empty() );

    /* return oldest pending complete response */
    HTTPResponse new_response;
    new_response = complete_responses_.front();
    complete_responses_.pop();
    return new_response;
}
