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

    bool full_;
// Variable which indicates whether it is a full response/first chunk or an intermediate chunk  since str will vary with this */

public:
    /* constructor for full response or first chunked response */
    HTTPResponse( const std::vector< HTTPHeader > headers_, const std::string status, const std::string body, bool chunked_ = false )
        : complete_headers_( headers_ ),
          status_line_( status ),
          body_( body ),
          chunk_( chunked_ ),
          full_( true )
    {}

    /* default constructor to create temp in parser */
    HTTPResponse( void )
        : complete_headers_(),
          status_line_(),
          body_(),
          chunk_( false ),
          full_()
    {}

    /* constructor for intermediate/last chunk */
    HTTPResponse( const std::string body )
        : complete_headers_(),
          status_line_(),
          body_( body ),
          chunk_( true ),
          full_( false )
    {}

    std::string str( void ) {
        std::string response;

        /* check if response is a full response/first chunk or intermediate/last chunk */
        if ( full_ ) {
            /* add request line to response */
            response.append( status_line_ + "\r\n" );

            /* iterate through headers and add "key: value\r\n" to response */
            for ( const auto & header : complete_headers_ ) {
                response.append( header.key() + ": " + header.value() + "\r\n" );
            }
            response.append( "\r\n" );
        }

        /* add body to response */
        response.append( body_ );
        return response;
    }
};

#endif /* HTTP_RESPONSE_HH */
