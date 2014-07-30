/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BULK_PARSER_HH
#define BULK_PARSER_HH

#include <string>

#include "int32.hh"

/* class to print an HTTP request/response to screen */

class BulkParser
{
private:
    Integer32 request_proto_size;
    Integer32 response_proto_size;
    std::string buffer;
    enum {REQ_SIZE, REQ, RES_SIZE, RES} state_;
public:
    BulkParser()
        : request_proto_size(),
          response_proto_size(),
          buffer(),
          state_( REQ_SIZE )
    {};
    void parse( const std::string & response );
};

#endif /* BULK_PARSER_HH */
