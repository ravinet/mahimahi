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
        if ( argc < 3 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " recorded_directory new_directory" );
        }

        /* clean directory name */
        string directory = argv[ 1 ];

        string new_dir = argv[ 2 ];

        if ( directory.empty() ) {
            throw Exception( argv[ 0 ], "directory name must be non-empty" );
        }

        /* make sure directory ends with '/' so we can prepend directory to file name for storage */
        if ( directory.back() != '/' ) {
            directory.append( "/" );
        }

        if ( new_dir.back() != '/' ) {
            new_dir.append( "/" );
        }

        make_directory( new_dir );

        const vector< string > files = list_directory_contents( directory  );

        for ( const auto filename : files ) {
            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );

            MahimahiProtobufs::RequestResponse protobuf;
            if ( not protobuf.ParseFromFileDescriptor( fd.num() ) ) {
                throw Exception( filename, "invalid HTTP request/response" );
            }
            string first_line = protobuf.request().first_line();

            //cout << first_line << endl;

            string path_scheme = first_line.substr( first_line.find( "/" ) + 1 );

            string path = path_scheme.substr( 0, path_scheme.find( " " ) );

            string prev = new_dir;

            string curr = path;

            while ( curr.find( "/" ) != string::npos ) {
                //cout << "CURR IS: " << curr << endl;
                //cout << "considering " << prev + curr.substr(0, curr.find("/") + 1 ) << endl;
                if ( not check_directory_existence( prev + curr.substr( 0, curr.find( "/" ) + 1 ) ) ) {
                    make_directory( prev + curr.substr( 0, curr.find( "/" )  + 1 ) );
                }
                string temp = curr.substr( curr.find( "/" ) + 1);
                string done = curr.substr( 0, curr.find( "/" ) + 1 );
                curr = temp;
                prev = prev + done;
            }

            if ( path == "" ) {
                path = "index.html";
            }

            if ( path.find( "?" ) != string::npos ) {
                path =  new_dir + path.substr( 0, path.find( "?" ) );
            } else {
                path = new_dir + path;
            }

            //cout << path << endl;

            FileDescriptor messages( SystemCall( "open request/response protobuf " + path + "\n", open(path.c_str(), O_WRONLY | O_CREAT, 00700 ) ) );
            
            messages.write( string(protobuf.response().body()) );
            

        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
