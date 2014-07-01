/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

#include "archive.hh"
#include "exception.hh"
#include "util.hh"
#include "http_header.hh"
#include "local_http_message.hh"
#include "timestamp.hh"

using namespace std;

vector< pair< HTTP_Record::http_message, string > > Archive::pending_ = vector< pair< HTTP_Record::http_message, string > >();

mutex Archive::archive_mutex;

condition_variable Archive::cv;

bool Archive::bulk_parsed = false;

string get_header_value( const string & header_to_find, const HTTP_Record::http_message & req )
{
    for ( int i = 0; i < req.headers_size(); i++ ) { /* iterate through headers until match */
        HTTPHeader current_header( req.headers( i ) );
        if ( HTTPMessage::equivalent_strings( current_header.key(), header_to_find ) ) { /* header field matches */
                return current_header.value();
        }
    }
    /* no match */
    return string( "" );
}

/* compare specific env header value and stored header value (if either does not exist, return true) */
bool check_headers( const string & stored_header, const HTTP_Record::http_message & saved_req, const HTTP_Record::http_message & new_req )
{
    string saved_value = get_header_value( stored_header, saved_req );
    string new_value = get_header_value( stored_header, new_req );

    if ( saved_value == new_value ) { /* header values match */
        /* XXX If the header does not exist for both, is that ok? they will both be "" and they will match */
        return true;
    } else {
        return false;
    }
}

/* return pair: first bool is whether request matches...second is whether it is a potential match or not.
cases: (true, false)- request matches and is a full match
       (true, true)- request matches and is potential match
       (false, true) and (false, false)- request does not match at all */
pair< bool, bool > compare_requests( const HTTP_Record::http_message & saved_req, const HTTP_Record::http_message & new_req )
{
    uint64_t query_loc_saved = saved_req.first_line().find( "?" );
    uint64_t query_loc_new = new_req.first_line().find( "?" );

    if ( ( ( query_loc_saved == string::npos ) and ( query_loc_new != string::npos ) ) or
        ( ( query_loc_saved != string::npos ) and ( query_loc_new == string::npos ) ) ) { /* one has query string and other doesn't */
        return make_pair( false, false );
    }

    if ( query_loc_new == std::string::npos ) { /* no query string: compare object name */
        if ( not ( new_req.first_line() == saved_req.first_line() ) ) {
            return make_pair( false, false );
        }
    } else { /* query string present: compare and if not match but object names matches, add to possibilities */
        if ( not ( new_req.first_line() == saved_req.first_line() ) ) { /* if they are not exactly equal */
            if ( new_req.first_line().substr( 0, query_loc_new ) == saved_req.first_line().substr( 0, query_loc_saved ) ) { /* request w/o query string matches */
                return make_pair( true, true );
            }
            else { /* request w/o query string doesn't match */
                return make_pair( false, false );
            }
        }
    }
    /* compare existing environment variables for request to stored header values */
    if ( not check_headers( "Accept_Encoding", saved_req, new_req ) ) { return make_pair( false, false ); }
    if ( not check_headers( "Accept_Language", saved_req, new_req ) ) { return make_pair( false, false ); }
    if ( not check_headers( "Host", saved_req, new_req ) ) { return make_pair( false, false ); }

    /* all compared fields match */
    return make_pair( true, false );
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

/* return response of request with longest matching substring to current request */
string closest_match( const vector< pair< HTTP_Record::http_message, string > > & possible, const HTTP_Record::http_message & new_req )
{
    vector< int > longest_str;
    for ( unsigned int i = 0; i < possible.size(); i++ ) {
        string current_req_line = possible.at( i ).first.first_line();
        string new_req_line = new_req.first_line();
        longest_str.emplace_back( longest_substr( new_req_line, current_req_line ) );
    }
    vector< int >::iterator result = max_element( begin( longest_str ), end( longest_str ) );
    int max_index = distance( begin( longest_str ), result );
    return possible.at( max_index ).second;
}

/* need function which, given an http_message (request), iterates through pending_ and compares it to the stored requests and if it finds a match, returns the match...else it returns "" */
string Archive::get_corresponding_response( const HTTP_Record::http_message & new_req )
{
    vector< pair< HTTP_Record::http_message, string > > potential_matches;

    for ( unsigned int i = 0; i < pending_.size(); i++ ) { /* iterate through pending_ to see if any requests match */
        pair< bool, bool > res = compare_requests( pending_.at( i ).first, new_req );
        if ( res.first == true ) { /* request either matches exactly or is a potential match */
            if ( res.second ==  false ) { /* full match */
                return pending_.at( i ).second;
            } else { /* potential match */
                potential_matches.emplace_back( pending_.at( i ) );
            }
        }
    }

    /* we didn't find an exact match...handle potential matches */
    if ( potential_matches.size() == 0 ) { /* no potential matches */
        return string( "" );
    } else { /* return response of potential match with largest shared substring in request line */
        return closest_match( potential_matches, new_req );
    }
}

void Archive::add_request( const HTTP_Record::http_message & request )
{
    unique_lock<mutex> archive_lck( Archive::archive_mutex);
    pending_.emplace_back( make_pair( request, "pending" ) );
}

void Archive::add_response( const string & response, const size_t position )
{
    unique_lock<mutex> archive_lck( Archive::archive_mutex );
    assert( pending_.size() > position );
    pending_.at( position ).second = response;
}

bool Archive::request_pending( const HTTP_Record::http_message & new_req )
{
    unique_lock<mutex> archive_lck( Archive::archive_mutex );
    if ( get_corresponding_response( new_req ) == "pending" ) {
        return true;
    }
    return false;
}

bool Archive::have_response( const HTTP_Record::http_message & new_req )
{
    unique_lock<mutex> archive_lck( Archive::archive_mutex );
    string res = get_corresponding_response( new_req );
    if ( res == "" or res == "pending" ) { /* either request not listed or response pending...either way no response */
        if ( bulk_parsed ) { /* finished parsing bulk response so don't bother sending request to remote proxy */
            return true;
        }
        return false;
    }
    return true;
}

string Archive::corresponding_response( const HTTP_Record::http_message & new_req )
{
    unique_lock<mutex> archive_lck( Archive::archive_mutex );
    /* XXX should we assert that this is not pending or empty string since the flow user should follow is check if pending and if not, check if there is a response...for now, will assert */
    string res = get_corresponding_response( new_req );
    //assert( res != "" );
    //assert( res != "pending" );

    if ( res == "" or res == "pending" ) {
        if ( bulk_parsed ) { /* finished parsing bulk response so send back same response as remote proxy which also would not find request */
            string no_match = "HTTP/1.1 200 OK\r\nContent-Type: Text/html\r\nConnection: close\r\nContent-Length: 24\r\n\r\nCOULD NOT FIND AN OBJECT";
            return no_match;
        }
    }
    return res;
}

void Archive::wait( void )
{
    unique_lock<mutex> pending_lock( Archive::archive_mutex );
    Archive::cv.wait( pending_lock );
}

void Archive::finished_parsing_bulk( void )
{
    bulk_parsed = true;
}
