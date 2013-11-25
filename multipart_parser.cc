/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */ 

#include <string>
#include <iostream>
#include <stdlib.h>
#include <typeinfo>

#include "multipart_parser.hh"
#include "tokenize.hh"
#include "ezio.hh"

using namespace std;

static const std::string CRLF = "\r\n";

size_t MultipartBodyParser::parse( const string & str, const string & boundary ) {
    buffer_.clear();
    buffer_.append( previous_header_ );
    buffer_.append( str );

    cout << "MULTIPART" << endl;
    while ( not buffer_.empty() ) { /* if more in buffer, try to parse entire part */
        if ( part_header_present() ) { /* if we have full part header, try to get next part size */
            if( !part_pending_ ) { /* finished previous part so get next part size */
                pop_part_header();
            }
        }
        else { /* don't have enough headers to get part size, so store header part */
            if ( !part_pending_ ) {
                previous_header_.append( buffer_.substr( previous_header_.size() ) );
                return std::string::npos;
            }
        }
        if ( buffer_.size() >= part_size_ ) { /* we have enough to finish part */
            body_in_progress_.append( buffer_.substr( 0, part_size_ ) );
            buffer_.replace( 0, part_size_, string() );
            part_pending_ = false;
            if ( buffer_.find( boundary + "--" ) == 0 ) { /* last part (include -- and CRLF in size) */
                body_in_progress_.append( buffer_.substr(0, boundary.size() + 4 ) );
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

bool MultipartBodyParser::part_header_present( void )
{
    auto tokens = split( buffer_, CRLF );
    if ( tokens.size() >= 5 ) { return true; }
    return false;
}

/* pops part header and returns part size */
void MultipartBodyParser::pop_part_header( void )
{
    /* Boundary starting new part */
    string boundary_line = buffer_.substr( 0, buffer_.find( CRLF ) + CRLF.size() );
    body_in_progress_.append( boundary_line );
    buffer_.replace( 0, boundary_line.size(), string() );

    /* Content-Type: */
    string type_line = buffer_.substr( 0, buffer_.find( CRLF ) + CRLF.size() );
    body_in_progress_.append( type_line );
    buffer_.replace( 0, type_line.size(), string() );

    /* Content-Range: */
    string range_line = buffer_.substr( 0, buffer_.find( CRLF ) + CRLF.size() );
    body_in_progress_.append( range_line );
    buffer_.replace( 0, range_line.size(), string() );

    /* Blank line separating part header from part body */
    string separator_line = buffer_.substr( 0, buffer_.find( CRLF ) + CRLF.size() );
    body_in_progress_.append( separator_line );
    buffer_.replace(0, separator_line.size(), string() );

    part_size_ =  get_part_size( range_line );

    /* if we used a previously stored header, don't include that in current body size */
    body_in_progress_.replace( 0, previous_header_.size(), string() );
    previous_header_.clear();
}

size_t MultipartBodyParser::get_part_size( const string & size_line )
{
    size_t range_size;

    if ( size_line.find( "bytes" ) == std::string::npos ) { /* size line does not say bytes */
        /* Content-Range: 100-200/1270 */
        size_t colon = size_line.find( ":" );
        size_t dash = size_line.find( "-", size_line.find( "-" ) + 1 );
        size_t range_start = colon + 2;
        size_t range_end = size_line.find( "/") - 1;
        cout << "DASH " << dash << " start: " << range_start << endl;
        string lower_bound = size_line.substr( range_start, dash - range_start );
        string upper_bound = size_line.substr( dash + 1, range_end - dash );
        range_size = myatoi( upper_bound ) - myatoi( lower_bound ) + 1 + CRLF.size();
        cout << "LOWER BOUND: " << lower_bound << " UPPER BOUND: " << upper_bound << endl;
    } else {
        /* Content-range: bytes 500-899/8000 */
        size_t dash = size_line.find( "-", size_line.find( "-" ) + 1 );
        size_t range_start = size_line.find( "bytes" ) + 6;
        size_t range_end = size_line.find( "/" ) - 1;

        string lower_bound = size_line.substr( range_start, dash - range_start );
        string upper_bound = size_line.substr( dash + 1, range_end - dash );

        range_size = myatoi( upper_bound ) - myatoi( lower_bound ) + 1 + CRLF.size();
    }
    return range_size;
}
