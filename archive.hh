/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <mutex>
#include <condition_variable>

#include "http_record.pb.h"

#ifndef ARCHIVE_HH
#define ARCHIVE_HH

class Archive
{
private:
    static std::mutex archive_mutex;
    static std::condition_variable cv;

public:
    static std::vector< std::pair< HTTP_Record::http_message, std::string > > pending_;

    static std::string get_corresponding_response( const HTTP_Record::http_message & new_req );

    /* Add a request */
    static void add_request( const HTTP_Record::http_message & request );

    /* Add a response */
    static void add_response( const std::string & response, const size_t position );

    /* Do we have a matching request that is pending? */
    static bool request_pending( const HTTP_Record::http_message & new_req );

    /* Do we have a stored response for this request? */
    static bool have_response( const HTTP_Record::http_message & new_req );

    /* Return the corresponding response to the stored request (caller should first call have_response) */ 
    static std::string corresponding_response( const HTTP_Record::http_message & new_req );

    static size_t num_of_requests( void ) { std::unique_lock<std::mutex> archive_lck( archive_mutex ); return pending_.size(); }

    static bool has_first_response( void )
    {
//        std::cout << "PENDING SIZE: " << pending_.size() << " second: " << pending_.at(0).second << std::endl;
        if ( pending_.size() > 0 and pending_.at(0).second != "pending" ) {
            return true;
        }
        return false;
    }

    static std::string first_response( void ) { return pending_.at(0).second; }

    static void wait( void );

    static void notify( void ) { Archive::cv.notify_all(); }

    static bool bulk_parsed;

    static void finished_parsing_bulk( void );
};

#endif
