/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "temp_file.hh"
#include "exception.hh"
#include "util.hh"

using namespace std;

vector<char> to_mutable( const string & str )
{
    vector< char > ret;
    for ( const auto & ch : str ) {
        ret.push_back( ch );
    }
    ret.push_back( 0 ); /* null terminate */

    return ret;
}

string from_mutable( const vector<char> & vec )
{
    string ret;

    for ( const auto & ch : vec ) {
        ret.push_back( ch );
    }

    return ret;
}

TempFile::TempFile( const string & filename_template )
    : mutable_temp_filename_( to_mutable( "/tmp/" + filename_template + ".XXXXXX" ) ),
      fd_( SystemCall( "mkstemp", mkstemp( &mutable_temp_filename_[ 0 ] ) ) ),
      filename_( from_mutable( mutable_temp_filename_ ) ),
      moved_away_( false )
{
    assert_not_root();

    /* check the filename */
    for ( const auto & ch : filename_ ) {
        if ( ch == 0 or ch == '/' ) {
            throw Exception( "mkstemp", "invalid character in returned tempfile name" );
        }
    }
}

TempFile::~TempFile()
{
    assert_not_root();

    if ( not moved_away_ ) {
        SystemCall( "unlink " + name(), unlink( name().c_str() ) );
    }
}

void TempFile::write( const string & contents )
{
    assert( not moved_away_ );

    fd_.write( contents );
}

TempFile::TempFile( TempFile && other )
    : mutable_temp_filename_( other.mutable_temp_filename_ ),
      fd_( move( other.fd_ ) ),
      filename_( other.filename_ ),
      moved_away_( false )
{
    other.moved_away_ = true;
}
