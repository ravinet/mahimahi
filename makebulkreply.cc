/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>

#include "util.hh"
#include "system_runner.hh"
#include "http_record.pb.h"
#include "exception.hh"
#include "file_descriptor.hh"
#include "http_message.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 3 ) {
            throw Exception( "Usage", string( argv[0] ) + " folder_with_recorded_content hostname(eg. www.sears.com)" );
        }
        string directory( argv[ 1 ] );
        if( directory.back() != '/' ) {
             directory.append( "/" );
        }
        string passed_header( argv[2] );
        vector< string > files;
        list_files( directory.c_str(), files );

        /* first write number of request/response pairs to the bulk response */
        uint32_t num_files = files.size();
        FileDescriptor bulkreply = SystemCall( "open", open( "bulkreply.proto", O_WRONLY | O_CREAT, 00700 ) );
        SystemCall( "write", write( bulkreply.num(), &num_files, 4 ) );

        unsigned int i;
        string first_response;

        /* first put GET / response to original host and remove this from list of files (do this since it should be first one listed) */
        for ( i = 0; i < num_files; i++ ) { // iterate through recorded files and find initial request protobuf
            FileDescriptor fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( fd.num() );

            HTTP_Record::http_message current_req;
            current_req = current_record.req();
            if (current_req.first_line() == "GET / HTTP/1.1\r\n" ) { /* could be first request */
                string host_header;
                /* iterate through headers and check if host matches given host (since later requests to other servers can be GET /) */
                for ( int j = 0; j < current_record.req().headers_size(); j++ ) {
                    HTTPHeader current_header( current_record.req().headers(j) );
                    if ( HTTPMessage::equivalent_strings( current_header.key(), "HOST" ) ) { /* store host header value */
                        host_header = current_header.value();
                           break;
                    }
                }
                if ( host_header.substr(0, host_header.find("\r\n")) == passed_header ) { /* host header value matches passed value so this is correct first request */
                    /* write request and size to bulk response */
                    string current_req;
                    current_record.req().SerializeToString( &current_req );
                    uint32_t req_size = current_req.size();
                    SystemCall( "write", write( bulkreply.num(), &req_size, 4 ) );
                    current_record.req().SerializeToFileDescriptor( bulkreply.num() );

                    /* store first response so we can add that first */
                    current_record.res().SerializeToString( &first_response );
                    break;
                }
            }
        }
        /* remove file for first request/response from list of files */
        files.erase( files.begin() + i );
        num_files = files.size();

        /* iterate through recorded files and for each request, append size and request to bulkreply */
        for ( i = 0; i < num_files; i++ ) {
            FileDescriptor fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( fd.num() );

            /* serialize request to string and obtain size */
            string current_req;
            current_record.req().SerializeToString( &current_req );
            uint32_t req_size = current_req.size();
            SystemCall( "write", write( bulkreply.num(), &req_size, 4 ) );
            current_record.req().SerializeToFileDescriptor( bulkreply.num() );
        }

        /* add first response (prepended with its size) to bulk response */
        uint32_t first_res_size = first_response.size();
        SystemCall( "write", write( bulkreply.num(), &first_res_size, 4 ) );
        bulkreply.write( first_response );

        /* iterate through recorded files and for each response, append size and response to bulkreply */
        for ( i = 0; i < num_files; i++ ) {
            FileDescriptor fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( fd.num() );

            /* serialize response to string and obtain size */
            string current_res;
            current_record.res().SerializeToString( &current_res );
            uint32_t res_size = current_res.size();
            SystemCall( "write", write( bulkreply.num(), &res_size, 4 ) );
            bulkreply.write( current_res );
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
