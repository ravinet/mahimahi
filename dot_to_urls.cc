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

int main( int argc, char *argv[] )
{
    /* should take a dot file and a folder
    1) read in dot file completely and add each url seen to a dictionary with an empty string as value
    2) for each url in the dictionary, iterate through the protobuf folder until we find a matching first line (stripping GET HTTP/1.1)
      -each time we find a match, add the url combining the host and first line to an array
      -when we 


     instead: lets just have a c++ program which takes a folder and spits out lines with the first_line stripped and the host
     then we can have a python program which takes this in as input and does the rest!

    */

    try {
        if ( argc < 1 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " recorded_folder" );
        }

        string folder = argv[ 1 ];

        const vector<string> files = list_directory_contents( folder );

        for ( const auto filename : files ) {
            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );
            MahimahiProtobufs::RequestResponse curr;
            if ( not curr.ParseFromFileDescriptor( fd.num() ) ) {
                throw runtime_error( filename + ": invalid HTTP request/response" );
            }

            string host = "";
            for ( int i = 0; i < curr.request().header_size(); i++ ) {
                HTTPHeader current_header( curr.request().header(i) );
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Host" ) ) {
                   host = current_header.value();
                }
            }

            string first_line = curr.request().first_line();
            cout << "FIRST LINE: " << first_line << " Host: " << host << endl;
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
