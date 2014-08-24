/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <ctime>

#include "archive.hh"
#include "http_request.hh"
#include "http_response.hh"
#include "http_header.hh"
#include "file_descriptor.hh"

using namespace std;

string remove_query( const string & request_line )
{
    const auto index = request_line.find( "?" );
    if ( index == string::npos ) {
        return request_line;
    } else {
        return request_line.substr( 0, index );
    }
}

int match_size( const string & first, const string & second )
{
    const auto max_match = min( first.size(), second.size() );
    for ( unsigned int i = first.find( "?" ); i < max_match; i++ ) {
        if ( first.at( i ) != second.at( i ) ) {
            return i;
        }
    }
    return max_match;
}

int date_diff( const string & date1, const string & date2 )
{
    /* returns the difference, in seconds, between date1 and date2 (can be negative if date2 is later than date1) */
    tm tm_date1;
    strptime( date1.c_str(), "%a, %d %b %Y %H:%M:%S", &tm_date1 );
    auto tp_date1 = chrono::system_clock::from_time_t( std::mktime( &tm_date1 ) );
    time_t tt_date1 = chrono::system_clock::to_time_t( tp_date1 );

    tm tm_date2;
    strptime( date2.c_str(), "%a, %d %b %Y %H:%M:%S", &tm_date2 );
    auto tp_date2 = chrono::system_clock::from_time_t( mktime( &tm_date2 ) );
    time_t tt_date2 = chrono::system_clock::to_time_t( tp_date2 );

    return difftime( tt_date1, tt_date2 );
}

int get_cache_value( const string & cache_header, const string & value_to_find )
{
    /* returns value of specific field of cache header (caller has checked that value field present) */
    int value = -1;
    size_t loc = cache_header.find( value_to_find );
    if ( loc != string::npos ) { /* we have the desired value */
        loc = loc + value_to_find.size() + 1;
        size_t comma = cache_header.find( ",", loc );
        if ( comma != string::npos ) { /* we have another cache field listed after this */
            value = myatoi( cache_header.substr( loc, ( comma - loc ) ).c_str() );
        } else { /* cache header ends after this field */
            value = myatoi( cache_header.substr( loc ).c_str() );
        }
    } else {
        throw Exception( "get_cache_val", value_to_find + " not present in cache header" );
    }
    return value;
}

Archive::Archive()
    : mutex_(),
      archive_()
{}

bool Archive::check_freshness( const HTTPRequest & new_request, const HTTPResponse & stored_response )
{
    /* current age represents how long since object in response was made */
    /* current_age = date_in_new_request - date_in_stored_response + Age header value in response (if present) */
    /* If no date present in request, assume age is 0 */
    int current_age = 0;
    if ( new_request.has_header( "Date" ) and stored_response.has_header( "Date" ) ) {
        current_age = date_diff( new_request.get_header_value( "Date" ), stored_response.get_header_value( "Date" ) );
        if ( stored_response.has_header( "Age" ) ) { /* stored header specifies age of object when response was sent */
            current_age = current_age + myatoi( stored_response.get_header_value( "Age" ) );
        }
    }

    /* max_freshness represents lifetime of response- initialize to traffic server default */
    int max_freshness = 86400;
    if ( stored_response.has_header( "Cache-Control" ) ) {
        string cache_control = stored_response.get_header_value( "Cache-Control" );
        if ( cache_control.find( "max-age" ) != string::npos ) { /* we have a max age */
            max_freshness = get_cache_value( cache_control, "max-age" );
        }
    } else if ( stored_response.has_header( "Expires" ) and stored_response.has_header( "Date" ) ) { /* we have expiration date */
        /* max_freshness = expires_date - date_in_stored_response */
        max_freshness = date_diff( stored_response.get_header_value( "Expires" ), stored_response.get_header_value( "Date" ) );
        if ( max_freshness < 0 ) { /* response has expired */
            max_freshness = 0;
        }
    } else if ( stored_response.has_header( "Date" ) and stored_response.has_header( "Last-Modified" ) ) {
        /* max_freshness = (date - last_modified) * 0.1 */
        max_freshness = date_diff( stored_response.get_header_value( "Date" ), stored_response.get_header_value( "Last-Modified" ) ) * 0.1;
        if ( max_freshness < 0 ) { /* response not consistent (last-modified can't be later than the date of response) */
            max_freshness = 0;
        }
    }

    /* compare freshness with new_request requirements */
    if ( new_request.has_header( "Cache-Control" ) ) { /* first check if client has freshness requirement */
        string cache_control = new_request.get_header_value( "Cache-Control" );
        if ( cache_control.find( "min-fresh" ) != string::npos ) { /* client has min requirement on freshness */
            int client_min_fresh = get_cache_value( cache_control, "min-fresh" );
            if ( client_min_fresh > ( max_freshness - current_age ) ) { /* remaining lifetime is less than what client wants */
                return false;
            } else {
                return true;
            }
        } else if ( cache_control.find( "max-stale" ) != string::npos ) { /* client willing to take stale response */
            int client_max_stale = get_cache_value( cache_control, "max-stale" );
            if ( client_max_stale < ( current_age - max_freshness ) ) { /* response fresh enough to send */
                return true;
            } else { /* response too stale */
                return false;
            }
        }
    }

    /* compare freshness with current age */
    if ( max_freshness >= current_age ) { /* response is still fresh */
        return true;
    } else { /* response stale */
        return false;
    }
}

pair< bool, string > Archive::find_request( const MahimahiProtobufs::HTTPMessage & incoming_req, const bool & check_fresh, const bool & pending_ok )
{
    /* iterates through requests in the archive and see if request matches any of them. if it does, return <true, response as string>
    otherwise, return <false, ""> */

    unique_lock<mutex> ul( mutex_ );

    pair< int, string > possible_match = make_pair( 0, "" );

    HTTPRequest request( incoming_req );

    for ( auto & x : archive_ ) {
        HTTPRequest curr( x.first );
        if ( remove_query( curr.first_line() ) == remove_query( request.first_line() ) ) { /* up to ? matches */
            if ( curr.get_header_value( "Host" ) == request.get_header_value( "Host" ) ) { /* host header matches */
                if ( curr.first_line() == request.first_line() ) { /* exact match */
                    HTTPResponse ret( x.second );
                    if ( ret.first_line() == "" ) { /* response is pending */
                        return make_pair( false, "" );
                    } else {
                        if ( check_fresh ) {
                            if ( check_freshness( request, ret ) ) { /* response is fresh */
                                return make_pair( true, ret.str() );
                            } else {
                                return make_pair( false, "" );
                            }
                        } else {
                            return make_pair( true, ret.str() );
                        }
                    }
                }
                if ( pending_ok ) {
                    /* possible match, but not exact */
                    int match_val = match_size( curr.first_line(), request.first_line() );
                    if ( match_val > possible_match.first ) { /* closer possible match */
                        HTTPResponse ret( x.second );
                        if ( ret.first_line() != "" ) { /* response is not pending */
                            if ( check_fresh ) {
                                if ( check_freshness( request, ret ) ) { /* response is fresh */
                                    possible_match = make_pair( match_val, ret.str() );
                                }
                            } else {
                                possible_match = make_pair( match_val, ret.str() );
                            }
                        }
                    }
                }
            }
        }
    }

    if ( possible_match.first != 0 ) { /* we have a possible match (same object name) */
        return make_pair( true, possible_match.second );
    }

    /* no match */
    return make_pair( false, "" );
}

int Archive::add_request( const MahimahiProtobufs::HTTPMessage & incoming_req )
{
    /* emplace to end of vector as <request, empty HTTPMessage> and return index */
    /* caller should have called find_request() to make sure request is not already there */

    unique_lock<mutex> ul( mutex_ );

    MahimahiProtobufs::HTTPMessage blank;
    int new_pos = archive_.size();
    archive_.emplace_back( make_pair( incoming_req, blank ) );
    return new_pos;
}

void Archive::add_response( const MahimahiProtobufs::HTTPMessage & response, const int & index )
{
    unique_lock<mutex> ul( mutex_ );

    archive_.at( index ).second = response;
}

void Archive::print( const HTTPRequest & req )
{
    unique_lock<mutex> ul( mutex_ );

    unlink( "archivestuff.txt" );

    string bulk_file_name = "archivestuff.txt";
    FileDescriptor bulkreply = SystemCall( "open", open( bulk_file_name.c_str(), O_WRONLY | O_CREAT, 00700 ) );
    bulkreply.write( "CLIENT REQUEST WAS: " + req.get_header_value( "Host" )  + " " + req.first_line() );
    bulkreply.write( "\n\nNumber of reqs/res: " + to_string( archive_.size() ) + "\n\n" );
    string to_write;
    for ( unsigned int i = 0; i < archive_.size(); i++ ) {
        HTTPRequest curr1( archive_.at( i ).first );
        HTTPResponse curr2( archive_.at( i ).second );
        to_write.append( curr1.first_line() + "\n");
        for ( const auto header : archive_.at( i ).second.header() ) {
            HTTPHeader curr( header );
            to_write.append( curr.str() + "\n" );
        }
        to_write.append( "\n\n" );

    }
    bulkreply.write( to_write );
}
