/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_MESSAGE_HH
#define HTTP_MESSAGE_HH

#include <string>
#include <vector>
#include <cassert>

#include "http_header.hh"

enum HTTPMessageState { FIRST_LINE_PENDING, HEADERS_PENDING, BODY_PENDING, COMPLETE };

/* helper for parsers */
static const std::string CRLF = "\r\n";

class HTTPMessage
{
private:
    /* first member of pair specifies whether body size is known in advance,
       and second member is size (if known in advance) */
    /* this is calculated by calculate_expected_body_size() */
    std::pair< bool, size_t > expected_body_size_ { false, -1 };

    /* calculating the body size must be implemented by request or response */
    virtual void calculate_expected_body_size( void ) = 0;

    /* bodies with size not known in advance must be handled by subclass */
    virtual size_t read_in_complex_body( const std::string & str ) = 0;

    /* connection closed while body was pending */
    virtual void eof_in_body( void ) = 0;

protected:
    /* request line or status line */
    std::string first_line_ {};

    /* request/response headers */
    std::vector< HTTPHeader > headers_ {};

    /* body may be empty */
    std::string body_ {};

    /* state of an in-progress request or response */
    HTTPMessageState state_ { FIRST_LINE_PENDING };

    /* used by subclasses to set the expected body size */
    void set_expected_body_size( const bool is_known, const size_t value = -1 );

public:
    HTTPMessage() {}

    /* methods called by an external parser */
    void set_first_line( const std::string & str );
    void add_header( const std::string & str );
    void done_with_headers( void );
    size_t read_in_body( const std::string & str );
    void eof( void );

    /* getters */
    bool body_size_is_known( void ) const;
    size_t expected_body_size( void ) const;
    const HTTPMessageState & state( void ) const { return state_; }

    /* troll through the headers */
    bool has_header( const std::string & header_name ) const;
    const std::string & get_header_value( const std::string & header_name ) const;

    /* serialize the request or response as one string */
    std::string str( void ) const;
};

#endif /* HTTP_MESSAGE_HH */
