/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_RESPONSE_PARSER_HH
#define HTTP_RESPONSE_PARSER_HH

#include "http_message_sequence.hh"
#include "http_response.hh"
#include "http_request.hh"
#include "bytestream_queue.hh"

class HTTPResponseParser : public HTTPMessageSequence<HTTPResponse>
{
private:
    /* Need this to handle RFC 2616 section 4.4 rule 1 */
    std::queue< bool > requests_were_head_ {};

    void initialize_new_message( void ) override;

public:
    void new_request_arrived( const HTTPRequest & request );

    void parse( const std::string & buf, ByteStreamQueue & from_dest );

    bool parsing_step( ByteStreamQueue & from_dest );
};

#endif /* HTTP_RESPONSE_PARSER_HH */
