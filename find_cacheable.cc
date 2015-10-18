/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <paths.h>
#include <grp.h>
#include <cstdlib>
#include <fstream>
#include <resolv.h>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

#include "util.hh"
#include "system_runner.hh"
#include "http_response.hh"
#include "http_record.pb.h"
#include "file_descriptor.hh"

using namespace std;

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

    cerr << "date1: " << date1 << " date2: " << date2 << " diff: " << difftime( tt_date1, tt_date2) << " times: " << tt_date1 << " " << tt_date2 << endl;
    return difftime( tt_date1, tt_date2 );
    //return int(diff);
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

int main( int argc, char *argv[] )
{
    try {
        if ( argc < 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " directory" );
        }

        string directory = argv[ 1 ];

        const vector< string > files = list_directory_contents( directory );

        // total counts for objects and cacheable objects
        int total_obj = 0;
        int total_cacheable = 0;
        int total_js = 0;
        int js_cache = 0;

        for ( const auto filename : files ) {
            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );
            MahimahiProtobufs::RequestResponse curr;
            if ( not curr.ParseFromFileDescriptor( fd.num() ) ) {
                throw Exception( filename, "invalid HTTP request/response" );
            }
            MahimahiProtobufs::HTTPMessage old_one( curr.response() );
            bool cacheable = true;
            int freshness = 0;
            string header_used = "";
            string date_sent = "";
            string last_modified = "";
            total_obj = total_obj + 1;
            bool is_js = false;
            for ( int v = 0; v < old_one.header_size(); v++ ) {
                HTTPHeader curr_header( old_one.header(v) );
                if ( HTTPMessage::equivalent_strings( curr_header.key(), "Date" ) ) {
                    date_sent = curr_header.value();
                }
                if ( HTTPMessage::equivalent_strings( curr_header.key(), "Last-Modified" ) ) {
                    last_modified = curr_header.value();
                }
                if ( HTTPMessage::equivalent_strings( curr_header.key(), "Content-Type" ) ) {
                    if ( (curr_header.value().find("image") != string::npos) || (curr_header.value().find("png") != string::npos) || (curr_header.value().find("jpeg") != string::npos) || (curr_header.value().find("gif") != string::npos) ) {
                        total_js = total_js + 1;
                        is_js = true;
                    }
                }
            }
            for ( int i = 0; i < old_one.header_size(); i++ ) {
                HTTPHeader current_header( old_one.header(i) );
                // check if Cache-control header has value of no-cache or no-store...if yes, then not cacheable
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Cache-control" ) ) {
                    if ( (current_header.value().find("no-cache") != string::npos) || (current_header.value().find("no-store") != string::npos) ) {
                        cacheable = false;
                    }
                }
                // Pragma: no-cache means don't cache
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Pragma" ) ) {
                    if ( current_header.value().find("no-cache") != string::npos ) {
                        cacheable = false;
                    }
                }

                // Expires value can imply not-cacheable (if 0)
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Expires" ) ) {
                    if ( current_header.value() == "0" ) {
                        cacheable = false;
                    }
                }

                // use headers to calculate freshness---priority order is Cache-control (maxage), Expires, Last-modified
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Cache-control" ) ) {
                    if ( current_header.value().find("max-age") != string::npos ) {
                        header_used = "cache-control";
                        int max_age_pos = current_header.value().find("max-age=");
                        string remaining = current_header.value().substr(max_age_pos + 8);
                        size_t fin = remaining.find(",");
                        if ( fin != string::npos ) {
                            freshness = stoi(remaining.substr(0, fin));
                        } else {
                            size_t white = remaining.find(" ");
                            if ( white == string::npos ) {
                                freshness = stoi(remaining);
                            } else {
                                freshness = stoi(remaining.substr(0, white));
                            }
                        }
                    }
                }

                // Expires - Date is freshness (if not set by cache-control)
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Expires" ) ) {
                    if ( cacheable ) {
                        string exp_date = current_header.value();
                        if ( header_used != "cache-control" ) {
                            freshness = date_diff( exp_date, date_sent );
                            header_used = "expires";
                        }
                    }
                }

                // Last resort is 0.1(Date-Last-Modified)
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Expires" ) ) {
                    if ( cacheable ) {
                        string last_date = current_header.value();
                        if ( (header_used != "cache-control") && (header_used != "expires") ) {
                            if ( last_modified != "" ) {
                                freshness = date_diff( date_sent, last_modified ) * 0.1;
                                header_used = "last-modified";
                            }
                        }
                    }
                }
            }

            // if cacheable, then print filename and freshness...if not cacheable, then just print filename and not-cacheable (or freshness=0)
            if ( cacheable ) {
                if ( freshness > 0 ) {
                    cout << filename << " freshness=" << freshness << endl;
                    total_cacheable = total_cacheable + 1;
                    if ( is_js ) {
                        js_cache = js_cache + 1;
                    }
                } else {
                    cout << filename << " freshness=0" << endl;
                }
            } else {
                cout << filename << " freshness=0" << endl;
            }
            //if ( (header_used == "last-modified") || (header_used == "expires") ) {
            //    if ( cacheable ) {
            //        cout << "ODD HEADER FOR: " << filename << endl;
            //    }
            //}
        }
        //float ratio = total_cacheable/float(total_obj);
        //cout << "TOTAL OBJECTS: " << total_obj << " and TOTAL CACHEABLE: " << total_cacheable << " RATIO: " << ratio << endl;
        //float js_ratio = js_cache/float(total_js);
        //cout << "TOTAL IMAGE: " << total_js << " and IMAGE CACHEABLE: " << js_cache << " RATIO: " << js_ratio << endl;
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
