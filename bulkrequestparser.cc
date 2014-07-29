/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <cassert>

#include "util.hh"
#include "exception.hh"
#include "file_descriptor.hh"
#include "int32.hh"
#include "http_record.pb.h"

using namespace std;

int main()
{
    try {
        FileDescriptor bulkreply = SystemCall( "open", open( "bulk_request.txt", O_RDONLY ) );

        string bulk_request( bulkreply.read() );

        /* Request protobuf size */
        Integer32 req_size = static_cast<Integer32>( bulk_request.substr( 0,4 ) );
        bulk_request = bulk_request.substr( 4 );
        cout << "Request protobuf size: " << req_size << endl;

        /* Print incoming request url (to be given to phantomjs) */
        string request_proto = bulk_request.substr( 0, req_size );
        bulk_request = bulk_request.substr( req_size );
        MahimahiProtobufs::BulkRequest request;
        request.ParseFromString( request_proto );
        string scheme = (request.scheme() == MahimahiProtobufs::BulkRequest_Scheme_HTTPS ? "https" : "http" );
        MahimahiProtobufs::HTTPMessage request_message = request.request();
        string hostname;
        for ( int i = 0; i < request_message.header_size(); i++ ) {
            if ( request_message.header( i ).key() == "Host" ) {
                hostname = request_message.header( i ).value();
            }
        }
        string url = scheme + "://" + hostname;
        cout << url << endl;

        /* Make sure the string is now empty */
        assert( bulk_request.size() == 0 );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
