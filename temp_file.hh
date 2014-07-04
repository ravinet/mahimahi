/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef TEMP_FILE_HH
#define TEMP_FILE_HH

#include <string>
#include <vector>

#include "file_descriptor.hh"

class TempFile
{
private:
    std::vector<char> mutable_temp_filename_;
    FileDescriptor fd_;
    std::string filename_;

    bool moved_away_;

public:
    TempFile( const std::string & filename_template );
    ~TempFile();

    const std::string & name( void ) { return filename_; }

    void write( const std::string & contents );

    /* ban copying */
    TempFile( const TempFile & other ) = delete;
    TempFile & operator=( const TempFile & other ) = delete;

    /* allow move constructor */
    TempFile( TempFile && other );

    /* ... but not move assignment operator */
    TempFile & operator=( TempFile && other ) = delete;
};

#endif
