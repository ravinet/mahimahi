/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */ 

#include <string>
#include <cassert>
#include <iostream>

#include "body_parser.hh"
#include "exception.hh"

using namespace std;

static const std::string CRLF = "\r\n";

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
        return chunked_parser.parse( str, trailers );

    case MULTIPART:
        return multipart_parser.parse( str, boundary );

    }
    throw Exception( "Body Type Not Set" );
    return 0;
}
