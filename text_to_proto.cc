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
        if ( argc < 3 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " modified_file replayshell_file" );
        }

        string modified_file = argv[ 1 ];
        string proto_file = argv[ 2 ];

        /* read in modified text file */
        string str, total;
        ifstream in;
        FileDescriptor fd( SystemCall( "open", open( modified_file.c_str(), O_RDONLY ) ) );
        in.open( modified_file.c_str() );
        getline( in, str );
        while ( in ) {
            total += (str + "\n");
            getline( in, str );
        }
        /* remove extra /n at the end */
        total = total.substr( 0, total.size() - 1 );

        MahimahiProtobufs::RequestResponse protobuf;

        {
            FileDescriptor old( SystemCall( "open ", open( proto_file.c_str(), O_RDONLY ) ) );

            /* store previous version (before modification) of req/res protobuf */
            if ( not protobuf.ParseFromFileDescriptor( old.num() ) ) {
                throw Exception( proto_file, "invalid HTTP request/response" );
            }
        }

        MahimahiProtobufs::RequestResponse final_protobuf;
        MahimahiProtobufs::HTTPMessage new_response( protobuf.response() );

        /* use modified text file as new response body */
        new_response.set_body( total );

        /* create new request/response pair using old request and new response */
        final_protobuf.set_ip( protobuf.ip() );
        final_protobuf.set_port( protobuf.port() );
        final_protobuf.set_scheme( protobuf.scheme() ); 
        final_protobuf.mutable_request()->CopyFrom( protobuf.request() );
        final_protobuf.mutable_response()->CopyFrom( new_response );

        /* delete previous version of protobuf file */
        if( remove( proto_file.c_str() ) != 0 ) {
            throw Exception( "Could not remove file: ", proto_file.c_str() );
        }

        FileDescriptor messages( SystemCall( "open", open( proto_file.c_str(), O_WRONLY | O_CREAT, 00600 ) ) );

        /* write new req/res protobuf (with modification) to the same file */
        if ( not final_protobuf.SerializeToFileDescriptor( messages.num() ) ) {
            throw Exception( "rewriting new protobuf", "failure to serialize new request/response pair" );
        } 
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
