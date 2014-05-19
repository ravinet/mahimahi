/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_RESPONSE_HH
#define HTTP_RESPONSE_HH

#include <memory>

#include "http_message.hh"
#include "body_parser.hh"

class HTTPResponse : public HTTPMessage
{
private:
    std::string status_code( void ) const;

    bool request_was_head_;

    /* required methods */
    void calculate_expected_body_size( void );
    size_t read_in_complex_body( const std::string & str );
    bool eof_in_body( void );

    std::unique_ptr< BodyParser > body_parser_;

public:
    HTTPResponse() : request_was_head_( false ), body_parser_( nullptr ) {};
    void set_request_was_head( void );
};

#endif /* HTTP_RESPONSE_HH */
