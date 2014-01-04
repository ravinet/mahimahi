/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>

#include <iostream>
#include <vector>
#include <dirent.h>

#include "util.hh"
#include "system_runner.hh"
#include "http_record.pb.h"
#include "http_header.hh"
#include "exception.hh"

using namespace std;

void list_files( const string & dir, vector< string > & files )
{
    DIR *dp;
    struct dirent *dirp;

    if( ( dp  = opendir( dir.c_str() ) ) == NULL ) {
        throw Exception( "opendir" );
    }

    while ( ( dirp = readdir( dp ) ) != NULL ) {
        if ( string( dirp->d_name ) != "." and string( dirp->d_name ) != ".." ) {
            files.push_back( dir + string( dirp->d_name ) );
        }
    }
    SystemCall( "closedir", closedir( dp ) );
}

/* compare specific env header value and stored header value (if one does not exist, return true) */
bool check_headers( const string & env_header, const string & stored_header, HTTP_Record::http_message & saved_req )
{
    string env_value;
    if ( getenv( env_header.c_str() ) == NULL ) {
        return true;
    }

    /* environment header was passed to us */
    env_value = string( getenv( env_header.c_str() ) );
    for ( int i = 0; i < saved_req.headers_size(); i++ ) {
        HTTPHeader current_header( saved_req.headers(i) );
        if ( current_header.key() == stored_header ) { /* compare to environment variable */
            if ( current_header.value().substr( 0, current_header.value().find( "\r\n" ) ) == getenv( env_header.c_str() ) ) {
                return true;
            } else {
                return false;
            }
        }
    }
    /* environment header not in stored header */
    return true;
}

/* compare request_line and certain headers of incoming request and stored request */
bool compare_requests( HTTP_Record::reqrespair & saved_record, vector< HTTP_Record::reqrespair > & possible_matches )
{
    HTTP_Record::http_message saved_req = saved_record.req();

    /* request line */
    string new_req = string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "REQUEST_URI" ) ) + " " + string ( getenv( "SERVER_PROTOCOL" ) ) + "\r\n";

    /* if query string present, compare. if not a match but object name matches, add to possibilities */
    uint64_t query_loc = saved_req.first_line().find( "?" );
    if ( query_loc == std::string::npos ) { /* no query string */
        if ( not ( new_req == saved_req.first_line() ) ) {
            return false;
        }
    } else { /* query string present */
        if ( not ( new_req == saved_req.first_line() ) ) {
            string no_query = string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "SCRIPT_NAME" ) );
            if ( no_query == saved_req.first_line().substr( 0, query_loc ) ) { /* request w/o query string matches */
                possible_matches.emplace_back( saved_record );
            }
            return false;
        }
    }

    /* compare existing environment variables for request to stored header values */
    if ( not check_headers( "HTTP_ACCEPT_ENCODING", "Accept_Encoding", saved_req ) ) { cerr << "ACCEPT ENCODING DIFFERENT" << endl; return false; }
    if ( not check_headers( "HTTP_ACCEPT_LANGUAGE", "Accept_Language", saved_req ) ) { cerr << "ACCEPT LANGUAGE DIFF" << endl; return false; }
    //if ( not check_headers( "HTTP_CONNECTION", "Connection", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_COOKIE", "Cookie", saved_req ) ) { return false; }
    if ( not check_headers( "HTTP_HOST", "Host", saved_req ) ) { cerr << "HOST DIFFERENT" << endl; return false; }
    //if ( not check_headers( "HTTP_REFERER", "Referer", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_USER_AGENT", "User-Agent", saved_req ) ) { return false; }
    return true;
}

int longest_substr( const string & str1, const string & str2 )
{
    /* http://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Longest_common_substring#C.2B.2B */

    if ( str1.empty() || str2.empty() ) {
        return 0;
    }

    vector< int > curr (str2.size() );
    vector< int > prev (str2.size() );
    vector< int > swap;
    int maxSubstr = 0;

    for ( unsigned int i = 0; i < str1.size(); i++ ) {
        for ( unsigned int j = 0; j < str2.size(); j++) {
            if ( str1[ i ] != str2[ j ] ) {
                curr.at( j ) = 0;
            } else {
                if ( i == 0 || j == 0 ) {
                    curr.at( j ) = 1;
                } else {
                    curr.at( j ) = 1 + prev.at( j-1 );
                }
                if( maxSubstr < curr.at( j ) ) {
                    maxSubstr = curr.at( j );
                }
            }
        }
        swap=curr;
        curr=prev;
        prev=swap;
    }
    return maxSubstr;
}

int closest_match( const vector< HTTP_Record::reqrespair > & possible )
{
    string req = string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "REQUEST_URI" ) ) + " " + string ( getenv( "SERVER_PROTOCOL" ) ) + "\r\n";
    int longest_str = -1;
    int index_of_best_match = 0;
    int match_length;
    for ( unsigned int i = 0; i < possible.size(); i++ ) {
        string current_req = possible.at( i ).req().first_line();
        if ( (match_length = longest_substr( req, current_req) ) > longest_str ) {
            longest_str = match_length;
            index_of_best_match = i;
        }
    }
    return index_of_best_match;
}


void return_message( const HTTP_Record::reqrespair & record )
{
    cout << record.res().first_line();
    for ( int j = 0; j < record.res().headers_size(); j++ ) {
        cout << record.res().headers( j );
    }
    cout << record.res().body() << endl;
}

int main()
{
    try {
        //cout << "Content-Type: text/html\r\n\r\n";
        /* print all environment variables */
        /*extern char **environ;
        for(char **current = environ; *current; current++) {
            cout << *current << endl << "<br />\n";
        }*/
        vector< string > files;
        list_files( getenv( "RECORD_FOLDER" ), files );
        vector< HTTP_Record::reqrespair > possible_matches;
        possible_matches.reserve( files.size() );
        unsigned int i;
        for ( i = 0; i < files.size(); i++ ) { /* iterate through files and call method which compares requests */
            int fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( fd );
            if ( compare_requests( current_record, possible_matches ) ) { /* requests match */
                return_message( current_record );
                SystemCall( "close", close( fd ) );
                break;
            }
            SystemCall( "close", close( fd ) );
        }
        if ( i == files.size() ) { /* no exact matches for request */
            if ( possible_matches.size() == 0 ) {
                cout << "HTTP/1.1 200 OK\r\n";
                cout << "Content-Type: Text/html\r\nConnection: close\r\n";
                cout << "Content-Length: 24\r\n\r\nCOULD NOT FIND AN OBJECT";
                throw Exception( "replaymyserver", "Can't find: " + string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "REQUEST_URI" ) ) );
                return EXIT_FAILURE;
            } else { /* for now, return first stored possibility */
                return_message( possible_matches.at( closest_match( possible_matches ) ) );
                throw Exception( "replayserver", "Chose longest query string match for: " + string( getenv( "REQUEST_URI" ) ) );
            }
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
