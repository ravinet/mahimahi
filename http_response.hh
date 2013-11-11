/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_RESPONSE_HH
#define HTTP_RESPONSE_HH

#include <vector>
#include <string>

#include "http_header.hh"

class HTTPResponse
{
private:
    std::vector< HTTPHeader > complete_headers_;

    std::string status_line_;

    std::string body_;

    bool chunk_;

    std::string chunk_size_line_;

public:
    HTTPResponse( const std::vector< HTTPHeader > headers_, const std::string status, const std::string body )
        : complete_headers_( headers_ ),
          status_line_( status ),
          body_( body ),
          chunk_( false ),
          chunk_size_line_()
    {}

    /* default constructor to create temp in parser */
    HTTPResponse( void )
        : complete_headers_(),
          status_line_(),
          body_(),
          chunk_( false ),
          chunk_size_line_()
    {}

    HTTPResponse( const std::string chunk_size_line, const std::string body )
        : complete_headers_(),
          status_line_(),
          body_( body ),
          chunk_( true ),
          chunk_size_line_( chunk_size_line )
    {}

    std::string str( void ) {
        std::string response;

        /* add request line to request */
        response.append( status_line_ + "\r\n" );

        /* iterate through headers and add "key: value\r\n" to request */
        for ( const auto & header : complete_headers_ ) {
            response.append( header.key() + ": " + header.value() + "\r\n" );
        }

        /* separate headers and body and then add body to request */
        response.append( "\r\n" + body_ );
        return response;
    }
};

#endif /* HTTP_RESPONSE_HH */
