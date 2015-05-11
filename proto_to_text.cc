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
        if ( argc < 3 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " replayshell_file output_file" );
        }

        string proto_file = argv[ 1 ];
        string new_file = argv[ 2 ];

        FileDescriptor fd( SystemCall( "open", open( proto_file.c_str(), O_RDONLY ) ) );

        /* read in previous req/res protobuf */
        MahimahiProtobufs::RequestResponse protobuf;
        if ( not protobuf.ParseFromFileDescriptor( fd.num() ) ) {
            throw Exception( proto_file, "invalid HTTP request/response" );
        }

        FileDescriptor messages( SystemCall( "open", open( new_file.c_str(), O_WRONLY | O_CREAT, 00600 ) ) );

        /* write just the response body to the specific file in readable format */
        messages.write( string( protobuf.response().body() ) );

        /* check if response is gzipped, if object is html or javascript, and if chunked */
        bool gzipped = false;
        bool chunked = false;
        string object_type;
        for ( int i = 0; i < protobuf.response().header_size(); i++ ) {
            HTTPHeader current_header( protobuf.response().header(i) );
            if ( HTTPMessage::equivalent_strings( current_header.key(), "Content-Encoding" ) ) {
                if ( current_header.value().find("gzip") != string::npos ) {
                    /* it is gzipped */
                    gzipped = true;
                }
            }
            if ( HTTPMessage::equivalent_strings( current_header.key(), "Content-Type" ) ) {
                if ( current_header.value().find( "javascript" ) != string::npos ) { /* javascript */
                    object_type = "javascript";
                } else if ( current_header.value().find( "html" ) != string::npos ) { /* html */
                    object_type = "html";
                } else {
                    object_type = "neither";
                }
            }

            if ( HTTPMessage::equivalent_strings( current_header.key(), "Transfer-Encoding" ) ) {
                if ( current_header.value().find( "chunked" ) != string::npos ) { /* chunked */
                    chunked = true;
                }
            }
        }

        /* check if this is 'GET /'- for now we assume that this is the top-level object */
        string top_html = "";
        if ( (protobuf.request().first_line() == "GET / HTTP/1.1") and (object_type == "html")) {
           top_html = "index";
        }

        /* get the name of the object */
        string html_name = "";
        if ( object_type == "html" ) {
            string complete_name = protobuf.request().first_line();
            string remove_first_space = complete_name.substr(complete_name.find(" ")+1);
            html_name = remove_first_space.substr(0, remove_first_space.find(" "));
        }
        //if ( html_name != "" ) {
        //    if ( html_name.at(0) == '/' ) {
        //        if ( html_name != "/" ) {
        //            html_name = html_name.substr(1);
        //        }
        //    }
        //}

        /* check if we found that it was gzipped, if not then print not gzipped */
        if ( gzipped ) {
            if ( chunked ) {
                cout << object_type << top_html << "chunked,gzipped,name=" << html_name << endl;
            } else {
                cout << object_type << top_html << ",gzipped,name=" << html_name << endl;
            }
        } else {
            if ( chunked ) {
                cout << object_type << top_html << ",chunked,not gzipped,name=" << html_name << endl;
            } else {
                cout << object_type << top_html << ",not gzipped,name=" << html_name << endl;
            }
        }



    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
