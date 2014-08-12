/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ARCHIVE_HH
#define ARCHIVE_HH

#include <string>
#include <utility>
#include <vector>

#include "http_record.pb.h"

class Archive
{
private:
    std::vector< std::pair< MahimahiProtobufs::HTTPMessage, MahimahiProtobufs::HTTPMessage > > archive_;

public:
    Archive();
    std::pair< bool, std::string > find_request( const MahimahiProtobufs::HTTPMessage & incoming_req );
    int add_request( const MahimahiProtobufs::HTTPMessage & incoming_req );
    void add_response( const MahimahiProtobufs::HTTPMessage & response, const int & index );
};

#endif /* ARCHIVE_HH */
