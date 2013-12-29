/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>

#ifndef TEMP_FILE_HH
#define TEMP_FILE_HH

static const std::string config_file = "/tmp/apache_config.XXXXXX";

class TempFile
{
private:
    std::string file_name;
    int fd_;
public:
    TempFile( const std::string & contents );
    ~TempFile();   

    const std::string name( void ) { return file_name; }

    void append( const std::string & contents );
};

#endif
