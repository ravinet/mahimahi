/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */ 

#include <string>
#include <iostream>

#include "chunked_parser.hh"

using namespace std;

static const std::string CRLF = "\r\n";

bool ChunkedBodyParser::have_complete_line( void ) const
{
    size_t first_line_ending = buffer_.find( CRLF );
    return first_line_ending != std::string::npos;
}

size_t ChunkedBodyParser::parse( const string & str, bool trailers ) {
    buffer_.clear();
    buffer_ = str;
    cout << "CHUNKED" << endl;
    while ( !buffer_.empty() ) { /* if more in buffer, try to parse entire chunk */
        if ( have_complete_line() and !chunk_pending_ ) { /* finished previous chunk so get next chunk size */
            get_chunk_size();
        }
        if ( buffer_.size() >= chunk_size_ ) { /* we have enough to finish chunk */
            body_in_progress_.append( buffer_.substr( 0, chunk_size_ ) );
            buffer_.replace( 0, chunk_size_, string() ); 
            chunk_pending_ = false;
            if ( chunk_size_ == CRLF.size() ) { /* last chunk (end of response) */
                if ( !handle_trailers( trailers ) ) {
                    return std::string::npos;
                }
                body_in_progress_.append( CRLF );
                size_t parsed_size = body_in_progress_.size();
                body_in_progress_.clear();
                return parsed_size;
            }
        } else { /* we don't have enough to finish chunk */
            chunk_size_ = chunk_size_ - buffer_.size();
            chunk_pending_ = true;
            body_in_progress_.clear();
            return std::string::npos;
        }
    }
    return std::string::npos;
}

void ChunkedBodyParser::get_chunk_size( void )
{ /* sets chunk size including CRLF after chunk */
    string size_line = buffer_.substr( 0, buffer_.find( CRLF ) );
    body_in_progress_.append( size_line + CRLF );

    chunk_size_ = strtol( size_line.c_str(), nullptr, 16 ) + CRLF.size();

    buffer_.replace( 0, buffer_.find( CRLF ) + CRLF.size(), string() );
}

bool ChunkedBodyParser::handle_trailers( bool trailers )
{
    if ( trailers ) {
        size_t crlf_loc;
        while ( ( crlf_loc = buffer_.find( CRLF ) ) != 0 ) { /* add trailer line to body */
            if ( not have_complete_line() ) {
                chunk_pending_ = true;
                body_in_progress_.clear();
                return false;
            }
            body_in_progress_.append( buffer_.substr( 0, crlf_loc + CRLF.size() ) );
            buffer_.replace( 0, crlf_loc + CRLF.size(), string() );
        }
    }
    return true;
}
