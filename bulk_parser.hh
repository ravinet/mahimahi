/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BULK_PARSER_HH
#define BULK_PARSER_HH

#include <string>

/* class to print an HTTP request/response to screen */

class BulkParser
{
public:
    BulkParser() {};
    void parse( const std::string & response );
};

#endif /* BULK_PARSER_HH */
