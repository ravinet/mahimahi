/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>
#include <algorithm>
#include <iostream>
#include <vector>

#include "util.hh"
#include "system_runner.hh"
#include "http_record.pb.h"
#include "http_header.hh"
#include "exception.hh"
#include "http_message.hh"

using namespace std;

/* compare specific env header value and stored header value (if either does not exist, return true) */
bool check_headers( const string & env_header, const string & stored_header, HTTP_Record::http_message & saved_req )
{
    if ( getenv( env_header.c_str() ) == NULL ) {
        return true;
    }

    string env_value;

    /* environment header was passed to us */
    env_value = string( getenv( env_header.c_str() ) );
    for ( int i = 0; i < saved_req.headers_size(); i++ ) {
        HTTPHeader current_header( saved_req.headers(i) );
        if ( HTTPMessage::equivalent_strings( current_header.key(), stored_header ) ) { /* compare to environment variable */
            if ( current_header.value().substr( 0, current_header.value().find( "\r\n" ) ) == getenv( env_header.c_str() ) ) {
                return true;
            } else {
                cerr << stored_header << " does not match environment variable" << endl;
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

    uint64_t query_loc = saved_req.first_line().find( "?" );
    if ( query_loc == std::string::npos ) { /* no query string: compare object name */
        if ( not ( new_req == saved_req.first_line() ) ) {
            return false;
        }
    } else { /* query string present: compare and if not match but object names matches, add to possibilities */
        if ( not ( new_req == saved_req.first_line() ) ) {
            string no_query = string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "SCRIPT_NAME" ) );
            if ( no_query == saved_req.first_line().substr( 0, query_loc ) ) { /* request w/o query string matches */
                possible_matches.emplace_back( saved_record );
            }
            return false;
        }
    }

    /* compare existing environment variables for request to stored header values */
    if ( not check_headers( "HTTP_ACCEPT_ENCODING", "Accept_Encoding", saved_req ) ) { return false; }
    if ( not check_headers( "HTTP_ACCEPT_LANGUAGE", "Accept_Language", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_CONNECTION", "Connection", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_COOKIE", "Cookie", saved_req ) ) { return false; }
    if ( not check_headers( "HTTP_HOST", "Host", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_REFERER", "Referer", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_USER_AGENT", "User-Agent", saved_req ) ) { return false; }

    /* all compared fields match */
    return true;
}

/* return size of longest substring match between two strings */
int longest_substr( const string & str1, const string & str2 )
{
    /* http://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Longest_common_substring#C.2B.2B */

    if ( str1.empty() or str2.empty() ) {
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

/* return index of stored request_line with longest matching substring to current request */
int closest_match( const vector< HTTP_Record::reqrespair > & possible )
{
    string req = string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "REQUEST_URI" ) ) + " " + string ( getenv( "SERVER_PROTOCOL" ) ) + "\r\n";
    vector< int > longest_str;
    for ( unsigned int i = 0; i < possible.size(); i++ ) {
        string current_req = possible.at( i ).req().first_line();
        longest_str.emplace_back( longest_substr( req, current_req ) );
    }
    vector< int >::iterator result = max_element( begin( longest_str ), end( longest_str ) );
    return distance( begin( longest_str ), result );
}

/* write response to stdout (using no-parsed headers for apache ) */
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
        vector< string > files;
        list_files( getenv( "RECORD_FOLDER" ), files );
        vector< HTTP_Record::reqrespair > possible_matches;
        possible_matches.reserve( files.size() );
        unsigned int i;
        for ( i = 0; i < files.size(); i++ ) { /* iterate through recorded files and compare requests to incoming req*/
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
            if ( possible_matches.size() == 0 ) { /* no potential matches */
                cout << "HTTP/1.1 200 OK\r\n";
                cout << "Content-Type: Text/html\r\nConnection: close\r\n";
                cout << "Content-Length: 24\r\n\r\nCOULD NOT FIND AN OBJECT";
                throw Exception( "replayserver", "Can't find: " + string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "REQUEST_URI" ) ) );
            } else { /* return possible match with largest shared substring */
                return_message( possible_matches.at( closest_match( possible_matches ) ) );
            }
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
