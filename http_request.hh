/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_REQUEST_HH
#define HTTP_REQUEST_HH

#include "http_message.hh"

class HTTPRequest : public HTTPMessage
{
private:
    /* for a request, will always be known */
    void calculate_expected_body_size( void );

    /* we have no complex bodies */
    size_t read_in_complex_body( const std::string & str );

    /* connection closed while body was pending */
    void eof_in_body( void );

public:
    using HTTPMessage::HTTPMessage;

    bool is_head( void ) const;
};

#endif /* HTTP_REQUEST_HH */
