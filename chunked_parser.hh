/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CHUNKED_BODY_PARSER_HH
#define CHUNKED_BODY_PARSER_HH

#include "body_parser.hh"
#include "exception.hh"

class ChunkedBodyParser : public BodyParser
{
private:
    uint32_t get_chunk_size(const std::string & chunk_hdr) const;
    bool parse_byte(char byte);
    std::string parser_buffer_ {""};
    uint32_t current_chunk_size_ {0};
    enum {CHUNK_HDR, CHUNK, TRAILER} state_ {CHUNK_HDR};
    bool trailers_enabled_ {false};

public:
    std::string::size_type read( const std::string & ) override;

    bool eof( void ) override
    {
        throw Exception( "ChunkedBodyParser", "invalid EOF in the middle of body" );
        return false;
    };
    ChunkedBodyParser(bool t_trailers_enabled) : trailers_enabled_( t_trailers_enabled ) {};
};

#endif /* CHUNKED_BODY_PARSER_HH */
