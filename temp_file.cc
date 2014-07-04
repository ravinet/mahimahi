/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cassert>

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

TempFile::TempFile( const string & filename_template )
    : mutable_temp_filename_( to_mutable( "/tmp/" + filename_template + ".XXXXXX" ) ),
      fd_( SystemCall( "mkstemp", mkstemp( &mutable_temp_filename_[ 0 ] ) ) ),
      moved_away_( false )
{}

TempFile::~TempFile()
{
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
      moved_away_( false )
{
    other.moved_away_ = true;
}

string TempFile::name( void ) const
{
    assert( mutable_temp_filename_.size() > 1 );
    return string( mutable_temp_filename_.begin(), mutable_temp_filename_.end() - 1 );
}
