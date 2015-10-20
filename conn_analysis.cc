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
    try {
        if ( argc < 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " replayshell_file" );
        }

        string proto_file = argv[ 1 ];
        string new_file = argv[ 2 ];

        FileDescriptor fd( SystemCall( "open", open( proto_file.c_str(), O_RDONLY ) ) );

        /* read in previous req/res protobuf */
        MahimahiProtobufs::RequestResponse protobuf;
        if ( not protobuf.ParseFromFileDescriptor( fd.num() ) ) {
            throw Exception( proto_file, "invalid HTTP request/response" );
        }

        cout << protobuf.clientip();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
