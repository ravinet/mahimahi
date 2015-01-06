/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "http_memory_store.hh"
#include "http_record.pb.h"
#include "file_descriptor.hh"
#include "http_header.hh"
#include "int32.hh"
#include "file_descriptor.hh"
#include "google/protobuf/io/gzip_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

using namespace std;

HTTPMemoryStore::HTTPMemoryStore()
    : mutex_(),
      requests(),
      responses()
{}

string remove_query_string( const string & request_line )
{
    const auto index = request_line.find( "?" );
    if ( index == string::npos ) {
        return request_line;
    } else {
        return request_line.substr( 0, index );
    }
}

bool compare_requests( const HTTPRequest request1, const HTTPRequest request2 )
{
    /* requests are same if host header and first line match (allow query string differences) */
    string request1_cookie = "";
    string request2_cookie = "";
    if ( request1.has_header( "Cookie" ) ) {
        request1_cookie = request1.get_header_value( "Cookie" );
    }
    if ( request2.has_header( "Cookie" ) ) {
        request2_cookie = request2.get_header_value( "Cookie" );
    }

    bool cookies_compatible = false;
    if ( request1_cookie == "" and request2_cookie == "" ) { /* neither request has cookie header */
        cookies_compatible = true;
    }
    if ( request1_cookie != "" and request2_cookie != "" ) { /* both requests have cookie header */
        cookies_compatible = true;
    }

    size_t last_slash1 = request1.first_line().find_last_of( "/" );
    size_t last_slash2 = request2.first_line().find_last_of( "/" );
    string before_slash1 = request1.first_line().substr( 0, last_slash1 );
    string before_slash2 = request2.first_line().substr( 0, last_slash2 );

    size_t first_semi1 = request1.first_line().find_first_of( ";" );
    size_t first_semi2 = request2.first_line().find_first_of( ";" );
    string before_semi1 = request1.first_line().substr( 0, first_semi1 );
    string before_semi2 = request2.first_line().substr( 0, first_semi2 );

    if ( ( remove_query_string( request1.first_line() ) == remove_query_string( request2.first_line() ) ) or
         ( before_slash1 == before_slash2 ) or ( before_semi1 == before_semi2 ) ) { /* up to query string matches */
        if ( request1.get_header_value( "Host" ) == request2.get_header_value( "Host" ) ) { /* host header matches */
            if ( cookies_compatible ) {
                return true;
            }
        }
    }

    return false;
}

void HTTPMemoryStore::save( const HTTPResponse & response, const Address & server_address __attribute__ ( ( unused ) ) )
{
    unique_lock<mutex> ul( mutex_ );

    /* Add the current request to requests BulkMessage and current response to responses BulkMessage */
    requests.add_msg()->CopyFrom( response.request().toprotobuf() );
    responses.add_msg()->CopyFrom( response.toprotobuf() );
}

void HTTPMemoryStore::serialize_to_socket( Socket && client, MahimahiProtobufs::BulkRequest & bulk_request )
{
    /* bulk response format:
       length-value pair for requests protobuf,
       for each response:
         length-value pair for each response
     */
    unique_lock<mutex> ul( mutex_ );

    /* make sure there is a 1 to 1 matching of requests and responses */
    assert( requests.msg_size() == responses.msg_size() );

    if ( responses.msg_size() == 0 ) {
        const string error_message =
         "HTTP/1.1 404 Not Found" + CRLF +
         "Content-Type: text/plain" + CRLF + CRLF;
        client.write( static_cast<string>( Integer32( error_message.size() ) ) + error_message );
        return;
    }

    /* remove requests/respones which are in cache */
    MahimahiProtobufs::BulkMessage trimmed_requests;
    MahimahiProtobufs::BulkMessage trimmed_responses;
    for ( int j = 0; j < requests.msg_size(); j++ ) {
        bool request_cached = false;
        for ( int i = 0; i < bulk_request.cached_requests_size(); i++ ) {
            if ( compare_requests( bulk_request.cached_requests( i ), requests.msg( j ) ) ) { /* request already cached */
                request_cached = true;
            }
        }
        if ( not request_cached ) {
            trimmed_requests.add_msg()->CopyFrom( requests.msg( j ) );
            trimmed_responses.add_msg()->CopyFrom( responses.msg( j ) );
        }
    }

    /* Serialize all requests alone to a length-value pair for transmission */
    string all_requests;
    {
        google::protobuf::io::GzipOutputStream::Options options;
        options.format = google::protobuf::io::GzipOutputStream::GZIP;
        options.compression_level = 9;
        google::protobuf::io::StringOutputStream compressedStream( &all_requests );
        google::protobuf::io::GzipOutputStream compressingStream( &compressedStream, options );
        trimmed_requests.SerializeToZeroCopyStream( &compressingStream );
    }
    all_requests = static_cast<string>( Integer32( all_requests.size() ) ) + all_requests;

    /* Now make a set of length-value pairs, one for each response */
    string all_responses;
    for ( int i = 0; i < trimmed_responses.msg_size(); i++ ) {
        /* add response string size and response string */
        string current_response;
        {
            google::protobuf::io::GzipOutputStream::Options options;
            options.format = google::protobuf::io::GzipOutputStream::GZIP;
            options.compression_level = 9;
            google::protobuf::io::StringOutputStream compressedStream( &current_response );
            google::protobuf::io::GzipOutputStream compressingStream( &compressedStream, options );
            trimmed_responses.msg(i).SerializeToZeroCopyStream( &compressingStream );
        }

        all_responses = all_responses + static_cast<string>( Integer32( current_response.size() ) ) + current_response;
    }

    client.write( all_requests + all_responses );

    //cout << "TOTAL BULK: " << requests.msg_size() << " TOTAL CACHED: " << bulk_request.cached_requests_size() << " TOTAL TRIMMED: " << trimmed_requests.msg_size() << endl;
}
