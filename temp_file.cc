/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "temp_file.hh"
#include "exception.hh"
#include "util.hh"

using namespace std;

TempFile::TempFile( const string & contents, const string & filename )
    : file_name(),
      fd_()
{
    string name = "/tmp/" + filename + ".XXXXXX";
    char tmp_name[30];
    strcpy( tmp_name, name.c_str() );
    fd_ = SystemCall( "mkstemp", mkstemp( tmp_name ) );
    file_name = string( tmp_name );
    SystemCall( "write", write( fd_, contents.c_str(), contents.length() ) );
}

TempFile::~TempFile()
{
    SystemCall( "close", close( fd_ ) );
    SystemCall( "remove", remove( file_name.c_str() ) );
}

void TempFile::append( const string & contents )
{
    SystemCall( "write", write( fd_, contents.c_str(), contents.length() ) );
}
