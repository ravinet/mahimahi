/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <paths.h>
#include <grp.h>
#include <cstdlib>
#include <fstream>
#include <resolv.h>
#include <sys/stat.h>
#include <dirent.h>

#include "util.hh"
#include "system_runner.hh"
#include "http_response.hh"
#include "http_record.pb.h"
#include "file_descriptor.hh"

using namespace std;

bool check_directory_existence( const string & directory )
{
    struct stat sb;
    /* check if directory already exists */
    if ( !stat( directory.c_str(), &sb ) == 0 or !S_ISDIR( sb.st_mode ) )
    {
        if ( errno != ENOENT ) { /* error is not that directory does not exist */
            throw Exception( "stat" );
        }
        return false;
    }
    return true;
}

int main( int argc, char *argv[] )
{
    try {
        if ( argc < 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " directory" );
        }

        /* clean directory name */
        string directory = argv[ 1 ];

        if ( directory.empty() ) {
            throw Exception( argv[ 0 ], "directory name must be non-empty" );
        }

        const vector< string > files = list_directory_contents( directory  );

        bool https;

        for ( const auto filename : files ) {
            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );

            MahimahiProtobufs::RequestResponse protobuf;
            if ( not protobuf.ParseFromFileDescriptor( fd.num() ) ) {
                throw Exception( filename, "invalid HTTP request/response" );
            }
            if ( protobuf.port() == 443 ) { https = true; }
            
        }
        if ( https ) { cout <<"HTTPS" << endl; } else { cout << "HTTP" << endl;}
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
