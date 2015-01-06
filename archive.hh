/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ARCHIVE_HH
#define ARCHIVE_HH

#include <string>
#include <utility>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "http_record.pb.h"
#include "http_request.hh"
#include "http_response.hh"

class Archive
{
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector< std::pair< MahimahiProtobufs::HTTPMessage, MahimahiProtobufs::HTTPMessage > > archive_;
    bool check_freshness( const HTTPRequest & new_request, const HTTPResponse & stored_response );

public:
    Archive();
    void notify( void ) { cv_.notify_all(); }
    std::pair< bool, std::string > find_request( const MahimahiProtobufs::HTTPMessage & incoming_req, const bool & check_fresh = true, const bool & pending_ok = true );
    int add_request( const MahimahiProtobufs::HTTPMessage & incoming_req );
    void add_response( const MahimahiProtobufs::HTTPMessage & response, const int & index );
    void print( const HTTPRequest & req );
    void wait( void );
    void list_cache( MahimahiProtobufs::BulkRequest & bulk_request );
};

#endif /* ARCHIVE_HH */
