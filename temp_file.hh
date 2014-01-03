/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>

#ifndef TEMP_FILE_HH
#define TEMP_FILE_HH

static const std::string config_file = "apache_config";

class TempFile
{
private:
    std::string file_name;
    int fd_;
public:
    TempFile( const std::string & contents, const std::string & filename = config_file );
    ~TempFile();   

    const std::string name( void ) { return file_name; }

    void append( const std::string & contents );
};

#endif
