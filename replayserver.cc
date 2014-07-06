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
bool request_matches( const MahimahiProtobufs::RequestResponse & saved_record )
{
    /* match HTTP/HTTPS */
    switch ( saved_record.scheme() ) {
    case MahimahiProtobufs::RequestResponse_Scheme_HTTP:
        if ( getenv( "HTTPS" ) ) {
            return false;
        }
        break;
    case MahimahiProtobufs::RequestResponse_Scheme_HTTPS:
        if ( not getenv( "HTTPS" ) ) {
            return false;
        }
        break;
    default:
        throw Exception( "MahimahiProtobufs::RequestResponse", "unknown URL scheme" );
    }

    const HTTPRequest saved_request( saved_record.request() );

    /* request line */
    const string new_req = string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "REQUEST_URI" ) ) + " " + string ( getenv( "SERVER_PROTOCOL" ) );

    if ( new_req != saved_request.first_line() ) {
        cerr << "failed comparison: " << new_req << " vs. " << saved_request.first_line() << endl;
        return false;
    }

    /* compare existing environment variables for request to stored header values */
    if ( not header_match( "HTTP_ACCEPT_ENCODING", "Accept_Encoding", saved_request ) ) { return false; }
    if ( not header_match( "HTTP_ACCEPT_LANGUAGE", "Accept_Language", saved_request ) ) { return false; }
    if ( not header_match( "HTTP_HOST", "Host", saved_request ) ) { return false; }

    /* all compared fields match */
    return true;
}

int main( void )
{
    try {
        const vector< string > files = list_directory_contents( getenv( "RECORD_FOLDER" ) );
        vector< MahimahiProtobufs::RequestResponse > possible_matches;

        for ( const auto filename : files ) { /* iterate through recorded files and compare requests to incoming req*/
            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );
            MahimahiProtobufs::RequestResponse current_record;
            current_record.ParseFromFileDescriptor( fd.num() );
            if ( request_matches( current_record ) ) { /* requests match */
                cout << HTTPResponse( current_record.response() ).str();
                return EXIT_SUCCESS;
            }
        }

        /* no exact matches for request */
        cout << "HTTP/1.1 200 OK\r\n";
        cout << "Content-Type: text/html\r\nConnection: close\r\n";
        cout << "Content-Length: 24\r\n\r\nCOULD NOT FIND AN OBJECT";
        throw Exception( "replayserver", "Can't find: " + string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "REQUEST_URI" ) ) );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
