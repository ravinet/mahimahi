/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */ 

#include <string>
#include <cassert>
#include <iostream>

#include "body_parser.hh"
#include "tokenize.hh"
#include "exception.hh"

using namespace std;

static const std::string CRLF = "\r\n";

bool BodyParser::have_complete_line( void ) const
{
    size_t first_line_ending = buffer_.find( CRLF );
    return first_line_ending != std::string::npos;
}

size_t BodyParser::read( const string & str, BodyType type, size_t expected_body_size, const string & boundary, bool trailers ) {
    buffer_.clear();
    buffer_ = str;
    switch ( type ) {
    case IDENTITY_KNOWN:
        cout << "KNOW SIZE" << endl;
        if ( str.size() < expected_body_size - body_in_progress_.size() ) { /* we don't have enough for full response */
            body_in_progress_.append( str );
            return std::string::npos;
        }
        /* we have enough for full response so reset */
        body_in_progress_.clear();
        return expected_body_size - body_in_progress_.size();

    case IDENTITY_UNKNOWN:
        cout << "IDENTITY UNKOWN" << endl;
        return std::string::npos;
    case CHUNKED:
        //cout << "CHUNKED" << endl;
        return chunked_parser.parse( str, trailers );
    case MULTIPART:
        cout << "MULTIPART" << endl;
        while ( not buffer_.empty() ) { /* if more in buffer, try to parse entire part */
            if ( part_header_present() ) { /* if we have full part header, try to get next part size */
                if( !part_pending_ ) { /* finished previous part so get next part size */
                    pop_part_header();
                }
            }
            if ( buffer_.size() >= part_size_ ) { /* we have enough to finish part */
                body_in_progress_.append( buffer_.substr( 0, part_size_ ) );
                buffer_.replace( 0, part_size_, string() );
                part_pending_ = false;
                if ( buffer_.find( boundary + "--" ) == 0 ) { /* last part */
                    size_t parsed_size = body_in_progress_.size();
                    body_in_progress_.clear();
                    return parsed_size;
                }
            } else { /* we don't have enough to finish part */
                part_size_ = part_size_ - buffer_.size();
                part_pending_ = true;
                body_in_progress_.clear();
                return std::string::npos;
            }
        }
        return std::string::npos;
    }
    throw Exception( "Body Type Not Set" );
    return 0;
}

bool BodyParser::part_header_present( void )
{
    auto tokens = split( buffer_, CRLF );
    if ( tokens.size() >= 5 ) { return true; }
    return false;
}

/* pops part header and returns part size */
void BodyParser::pop_part_header( void )
{
    string boundary_line = buffer_.substr( 0, buffer_.find( CRLF ) + CRLF.size() );
    body_in_progress_.append( boundary_line );
    buffer_.replace( 0, boundary_line.size(), string() );

    string type_line = buffer_.substr( 0, buffer_.find( CRLF ) + CRLF.size() );
    body_in_progress_.append( type_line );
    buffer_.replace( 0, type_line.size(), string() );

    string range_line = buffer_.substr( 0, buffer_.find( CRLF ) + CRLF.size() );
    body_in_progress_.append( range_line );
    buffer_.replace( 0, range_line.size(), string() );

    string separator_line = buffer_.substr( 0, buffer_.find( CRLF ) + CRLF.size() );
    body_in_progress_.append( separator_line );
    buffer_.replace(0, separator_line.size(), string() );

    part_size_ =  get_part_size( range_line );
}

size_t BodyParser::get_part_size( const string & size_line )
{
    /* Content-range: bytes 500-899/8000 */
    size_t dash = size_line.find( "-" );
    size_t range_start = size_line.find( "bytes" ) + 6;
    size_t range_end = size_line.find( "/" ) - 1;

    string lower_bound = size_line.substr( range_start, dash - range_start );
    string upper_bound = size_line.substr( dash + 1, range_end - dash );

    return strtol(upper_bound.c_str(), nullptr, 16 ) - strtol( lower_bound.c_str(), nullptr, 16 ) + CRLF.size();
}
