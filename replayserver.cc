/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "util.hh"
#include "http_record.pb.h"
#include "http_header.hh"
#include "exception.hh"
#include "http_request.hh"
#include "http_response.hh"
#include "file_descriptor.hh"

using namespace std;

string safe_getenv( const string & key )
{
    const char * const value = getenv( key.c_str() );
    if ( not value ) {
        throw Exception( "missing environment variable", key );
    }
    return value;
}

/* compare specific env header value and stored header value (if either does not exist, return true) */
bool header_match( const string & env_var_name,
                   const string & header_name,
                   const HTTPRequest & saved_response )
{
    /* case 1: neither header exists (OK) */
    if ( (not getenv( env_var_name.c_str() )) and (not saved_response.has_header( header_name )) ) {
        return true;
    }

    /* case 2: headers both exist (OK if values match) */
    if ( getenv( env_var_name.c_str() ) and saved_response.has_header( header_name ) ) {
        return saved_response.get_header_value( header_name ) == string( getenv( env_var_name.c_str() ) );
    }

    /* case 3: one exists but the other doesn't (failure) */
    return false;
}

/* compare request_line and certain headers of incoming request and stored request */
bool request_matches( const MahimahiProtobufs::RequestResponse & saved_record,
                      const string & request_line,
                      const bool is_https )
{
    const HTTPRequest saved_request( saved_record.request() );

    /* match HTTP/HTTPS */
    if ( is_https and (saved_record.scheme() != MahimahiProtobufs::RequestResponse_Scheme_HTTPS) ) {
        return false;
    }

    if ( (not is_https) and (saved_record.scheme() != MahimahiProtobufs::RequestResponse_Scheme_HTTP) ) {
        return false;
    }

    /* match first line */
    if ( request_line != saved_request.first_line() ) {
        return false;
    }

    /* match host header */
    if ( not header_match( "HTTP_HOST", "Host", saved_request ) ) {
        return false;
    }

    /* match user agent */
    if ( not header_match( "HTTP_USER_AGENT", "User-Agent", saved_request ) ) {
        return false;
    }

    /* success! */
    return true;
}

int main( void )
{
    try {
        const string recording_directory = safe_getenv( "RECORD_FOLDER" );
        const string request_line = safe_getenv( "REQUEST_METHOD" )
            + " " + safe_getenv( "REQUEST_URI" )
            + " " + safe_getenv( "SERVER_PROTOCOL" );
        const bool is_https = getenv( "HTTPS" );

        const vector< string > files = list_directory_contents( recording_directory );
        vector< MahimahiProtobufs::RequestResponse > possible_matches;

        for ( const auto filename : files ) {
            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );
            MahimahiProtobufs::RequestResponse current_record;
            if ( not current_record.ParseFromFileDescriptor( fd.num() ) ) {
                throw Exception( filename, "invalid HTTP request/response" );
            }

            if ( request_matches( current_record, request_line, is_https ) ) { /* requests match */
                cout << HTTPResponse( current_record.response() ).str();
                return EXIT_SUCCESS;
            }
        }

        /* no exact matches for request */
        cout << "HTTP/1.1 200 OK\r\n";
        cout << "Content-Type: text/plain\r\nConnection: close\r\n";
        cout << "Content-Length: 24\r\n\r\nCOULD NOT FIND AN OBJECT";
        throw Exception( "replayserver", "Can't find: " + request_line );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
