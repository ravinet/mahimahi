/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BULK_BODY_PARSER_HH
#define BULK_BODY_PARSER_HH

#include <memory>

#include "body_parser.hh"
#include "exception.hh"
#include "http_record.pb.h"

class BulkBodyParser : public BodyParser
{
private:
    /* total number of requests and responses left in bulk response (will start equal) */
    uint32_t requests_left_ {0};
    uint32_t responses_left_ {0};

    bool first_response_sent_ {false};

    uint32_t current_message_size_ {0};

    std::string parser_buffer_ {""};
    std::string::size_type acked_so_far_ {0};
    enum {RESPONSE_SIZE, MESSAGE_HDR, MESSAGE} state_ {RESPONSE_SIZE};

    int last_size_ {0};
public:
    std::string::size_type read( const std::string &, ByteStreamQueue & from_dest ) override;

    /* Follow item 2, Section 4.4 of RFC 2616 */
    bool eof( void ) override { return true; };

    BulkBodyParser() {};
};

#endif /* BULK_BODY_PARSER_HH */
