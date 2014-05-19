/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CHUNKED_BODY_PARSER_HH
#define CHUNKED_BODY_PARSER_HH

#include "body_parser.hh"
#include "exception.hh"

class ChunkedBodyParser : public BodyParser
{
private:
    std::string::size_type compute_ack_size(const std::string & haystack,
                                            const std::string & needle,
                                            std::string::size_type input_size);
    uint32_t get_chunk_size(const std::string & chunk_hdr) const;
    std::string parser_buffer_;
    uint32_t current_chunk_size_;
    std::string::size_type acked_so_far_;
    std::string::size_type parsed_so_far_;
    enum {CHUNK_HDR, CHUNK, TRAILER} state_;
    const bool trailers_enabled_;

public:
    std::string::size_type read( const std::string & );

    /* Follow item 2, Section 4.4 of RFC 2616 */
    bool eof( void ) { return true; };

    ChunkedBodyParser(bool t_trailers_enabled = false) : parser_buffer_(), current_chunk_size_( 0 ), acked_so_far_( 0 ), parsed_so_far_( 0 ), state_( CHUNK_HDR ), trailers_enabled_( t_trailers_enabled ) {};
};

#endif /* CHUNKED_BODY_PARSER_HH */
