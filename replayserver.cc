/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>
#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>

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
        string record_folder = string( getenv( "RECORD_FOLDER" ) );
        if ( check_folder_existence( record_folder ) ) {
            delete_directory( record_folder );
        }
        /* Load incoming request in recordshell with phantomjs */
        run( { "/usr/local/bin/recordshell", "/home/ravi/mahimahi/scriptphantom/", "/usr/bin/phantomjs",
               "--ignore-ssl-errors=true", "--ssl-protocol=TLSv1", "/home/ravi/mahimahi/headlessload.js", getenv( "SCRIPT_URI" ) } );
        /* Make bulk reply inside recorded folder */
        run( { "/usr/local/bin/makebulkreply", "/home/ravi/mahimahi/scriptphantom/", getenv( "HTTP_HOST" ) } );
        string bulk_file_name = record_folder + "bulkreply.proto";
        std::ifstream is(bulk_file_name, std::ifstream::binary);
        string str;
        if (is) {
            // get length of file:
            is.seekg(0, is.end);
            int length = is.tellg();
            is.seekg(0, is.beg);

            str.resize(length, ' '); // reserve space
            char* begin = &*str.begin();

            is.read(begin, length);
            is.close();
        }
        cout << "HTTP/1.1 200 OK\r\n";
        cout << "Content-Type: application/x-bulkreply\r\n\r\n";
        cout << str;

       return EXIT_SUCCESS;
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
