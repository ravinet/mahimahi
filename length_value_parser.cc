/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include "int32.hh"
#include "length_value_parser.hh"
#include "http_record.pb.h"

using namespace std;

pair< bool, string> LengthValueParser::parse( const string & response )
{
    buffer.append( response );

    switch ( state_ ) {
    case SIZE: {
        if ( buffer.size() < 4 ) { /* Don't have enough for request protobuf size */
            return make_pair( false, "" );
        }
        proto_size = static_cast<Integer32>( buffer.substr( 0,4 ) );
        buffer = buffer.substr( 4 );
        state_ = BODY;
    }
    /* FALLTHROUGH to BODY if you are done with SIZE */

    case BODY: {
        if ( buffer.size() < proto_size ) { /* Don't have enough for request protobuf */
            return make_pair( false, "" );
        }
        string message = buffer.substr( 0, proto_size );
        buffer = buffer.substr( proto_size );
        state_ = SIZE;
        return make_pair( true, message);
    }
    }
    return make_pair( false, "" );
}
