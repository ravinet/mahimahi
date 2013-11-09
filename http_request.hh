/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_REQUEST_HH
#define HTTP_REQUEST_HH

#include <vector>
#include <string>

#include "http_header.hh"

class HTTPRequest
{
private:
    std::vector< HTTPHeader > complete_headers_;

    std::string request_line_;

    std::string body_;

public:
    HTTPRequest( const std::vector< HTTPHeader > headers_, const std::string request, const std::string body )
        : complete_headers_( headers_ ),
          request_line_( request ),
          body_( body )
    {}

    /* default constructor to create temp in parser */
    HTTPRequest( void )
        : complete_headers_(),
          request_line_(),
          body_()
    {}

    std::string str( void ) {
        std::string request;

        /* add request line to request */
        request.append( request_line_ + "\r\n" );

        /* iterate through headers and add "key: value\r\n" to request */
        for ( const auto & header : complete_headers_ ) {
            request.append( header.key() + ": " + header.value() + "\r\n" );
        }

        /* separate headers and body and then add body to request */
        request.append( "\r\n" + body_ );
        return request;
    }
};

#endif /* HTTP_REQUEST_HH */
