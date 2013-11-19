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

bool HTTPResponseParser::four_full_lines( void )
{
    if ( not have_complete_line() ) { return false; }
    size_t second_crlf = internal_buffer_.substr( internal_buffer_.find( crlf ) + crlf.size(), internal_buffer_.size() - internal_buffer_.find( crlf ) + crlf.size() ).find( crlf );
    size_t third_crlf = internal_buffer_.substr( second_crlf + crlf.size(), internal_buffer_.size() - second_crlf + crlf.size() ).find( crlf );
    size_t fourth_crlf = internal_buffer_.substr( third_crlf + crlf.size(), internal_buffer_.size() - third_crlf + crlf.size() ).find( crlf );
    if ( second_crlf == std::string::npos or third_crlf == std::string::npos or fourth_crlf == std::string::npos ) { return false; }
    return true;
}

void HTTPResponseParser::parse( const string & buf )
{
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

        /* got line, so add it to pending request */
        response_in_progress_.set_status_line( pop_line() );
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
        if ( response_in_progress_.is_chunked() && update_ ) {
            if ( not have_complete_line() ) { return false; }
            string size_line( pop_line() );
            response_in_progress_.get_chunk_size( size_line );
            response_in_progress_.append_to_body( size_line + crlf );
        }
        if ( response_in_progress_.is_multipart() && update_ ) {
            if ( not four_full_lines() ) { return false; }
            assert( internal_buffer_.find( response_in_progress_.get_boundary() ) == 0 );
            response_in_progress_.append_to_body( pop_line() + crlf ); /* boundary */
            response_in_progress_.append_to_body( pop_line() + crlf ); /* content-type */
            string part_size( pop_line() );
            response_in_progress_.append_to_body( part_size + crlf ); /* part size */
            response_in_progress_.get_part_size( part_size );
            response_in_progress_.append_to_body( pop_line() + crlf ); /* CRLF separating part header from part body */
        }

        if ( internal_buffer_.size() < response_in_progress_.expected_body_size() ) {
            if ( response_in_progress_.is_chunked() or response_in_progress_.is_multipart() ) {
                update_ = false;
            }
            return false;
        }

        if ( response_in_progress_.is_chunked() ) { /* chunked */
            if ( response_in_progress_.expected_body_size() != 0 ) {
                response_in_progress_.append_to_body( internal_buffer_.substr( 0, response_in_progress_.expected_body_size() ) );
                response_in_progress_.append_to_body( crlf );
                internal_buffer_.replace( 0, response_in_progress_.expected_body_size() + crlf.size(), string() );
                update_ = true;
                return true;
            }
            else { /* last chunk */
                if ( response_in_progress_.has_header( "Trailer" ) ) { /* trailers present */
                    size_t crlf_loc;
                    while ( ( crlf_loc = internal_buffer_.find( crlf ) ) != 0 ) { /* add trailer line to body */
                        if ( not have_complete_line() ) { update_ = false; return false; }
                        response_in_progress_.append_to_body( internal_buffer_.substr( 0, crlf_loc + crlf.size() ) );
                        internal_buffer_.replace( 0, crlf_loc + crlf.size(), string() );
                    }
                }
                assert( internal_buffer_.find( crlf ) == 0 );
                response_in_progress_.append_to_body( crlf ); /* add final CRLF */
                internal_buffer_.replace(0, crlf.size(), string() );
                response_in_progress_.done_with_body();
                update_ = true;
                return true;
            }
        } else if ( response_in_progress_.is_multipart() ) { /* multipart */
            response_in_progress_.append_to_body( internal_buffer_.substr( 0, response_in_progress_.expected_body_size() ) );
            response_in_progress_.append_to_body( crlf );
            internal_buffer_.replace( 0, response_in_progress_.expected_body_size() + crlf.size(), string() );
            if ( have_complete_line() ) {
                if (internal_buffer_.find( response_in_progress_.get_boundary() + string( "--" ) ) == 0 ) { /* done */
                    response_in_progress_.done_with_body();
                }
                update_ = true;
            } else {
                update_ = false;
                return false;
            }
            return true;
        } else { /* not chunked or multipart */
            /* ready to finish the request */
            response_in_progress_.append_to_body( internal_buffer_.substr( 0, response_in_progress_.expected_body_size() ) );
            internal_buffer_.replace( 0, response_in_progress_.expected_body_size(), string() );
            assert( response_in_progress_.state() == RESPONSE_COMPLETE );
            return true;
        }
    case RESPONSE_COMPLETE:
        complete_responses_.push( response_in_progress_ );
        response_in_progress_ = HTTPResponse();
        return true;
    }

    assert( false );
    return false;
}
