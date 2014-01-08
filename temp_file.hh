/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <vector>

#include "file_descriptor.hh"

#ifndef TEMP_FILE_HH
#define TEMP_FILE_HH

static const std::string default_filename_template = "apache_config";

class TempFile
{
private:
    std::vector<char> mutable_temp_filename_;
    FileDescriptor fd_;
    std::string filename_;

    bool moved_away_;

public:
    TempFile( const std::string & filename_template = default_filename_template );
    ~TempFile();   

    const std::string & name( void ) { return filename_; }

    void write( const std::string & contents );

    TempFile( TempFile && other );
};

#endif
