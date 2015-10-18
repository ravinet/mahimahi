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
#include <iostream>
#include <fstream>

#include "util.hh"
#include "system_runner.hh"
#include "http_response.hh"
#include "http_record.pb.h"
#include "file_descriptor.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        if ( argc < 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " directory" );
        }

        string directory = argv[ 1 ];

        const vector< string > files = list_directory_contents( directory );

        for ( const auto filename : files ) {
            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );
            MahimahiProtobufs::RequestResponse curr;
            if ( not curr.ParseFromFileDescriptor( fd.num() ) ) {
                throw Exception( filename, "invalid HTTP request/response" );
            }
            MahimahiProtobufs::HTTPMessage old_one( curr.response() );
            bool found_encoding = false;
            bool is_img = false;
            for ( int i = 0; i < old_one.header_size(); i++ ) {
                HTTPHeader current_header( old_one.header(i) );
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Content-Encoding" ) ) {
                   found_encoding = true;
                   if ( current_header.value().find("gzip") == string::npos ) {
                        cout << old_one.first_line() << endl;
                        if ( old_one.first_line().find("200") != string::npos ) {
                            cout << "NOT GZIPPED: " << filename << endl;
                        }
                   }
                }
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Content-Type" ) ) {
                   if ( (current_header.value().find("jpeg") != string::npos) || (current_header.value().find("gif") != string::npos) || (current_header.value().find("png") != string::npos)) {
                        is_img = true;
                    }
                }
            }
            if ( !found_encoding ) {
                if ( !is_img ) {
                    //if ( old_one.first_line().find("200") != string::npos ) {
                        cout << "NOT GZIPPED: " << filename << endl;
                    //}
                }
            }
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
