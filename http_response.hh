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

    bool request_was_head_ { false };

    /* required methods */
    void calculate_expected_body_size( void ) override;
    size_t read_in_complex_body( const std::string & str, ByteStreamQueue & from_dest );
    bool eof_in_body( void ) override;

    std::unique_ptr< BodyParser > body_parser_ { nullptr };

    bool is_bulk_ { false };

public:
    void set_request_was_head( void );

    size_t read_in_body( const std::string & str, ByteStreamQueue & from_dest );

    bool is_bulk( void ) { return is_bulk_; }
};

#endif /* HTTP_RESPONSE_HH */
