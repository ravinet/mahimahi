#include <assert.h>
#include "ezio.hh"
#include "chunked_parser.hh"

using namespace std;

/* Take a chunk header and parse it assuming no folding */
uint32_t ChunkedBodyParser::get_chunk_size(const string & chunk_hdr) const
{
    /* Check that the chunk header ends with a CRLF */
    assert(chunk_hdr.substr( chunk_hdr.length() - 2, 2 ) == "\r\n");

    /* If there are chunk extensions, ';' terminates chunk size */
    auto pos = chunk_hdr.find(";");

    /* There are no ';'s, and hence no chunk externsions, CRLF terminates chunk size */
    if (pos == string::npos) {
        pos = chunk_hdr.find("\r\n");
    }

    /* Can't be npos even now */
    assert( pos != string::npos );

    /* Parse hex string */
    return myatoi(chunk_hdr.substr(0, pos), 16);
}

/* Byte-at-a-time parser, might be slow, but conceptually simpler */
bool ChunkedBodyParser::parse_byte( char byte )
{
    parser_buffer_.push_back( byte );
    switch (state_) {
    case CHUNK_HDR:
        /* if you have CRLF, get chunk size & transition to CHUNK/TRAILER */
        if (parser_buffer_.find("\r\n") != string::npos) {
            /* get chunk size */
            current_chunk_size_ = get_chunk_size( parser_buffer_ );

            /* Transition appropriately */
            state_ = (current_chunk_size_ == 0) ? TRAILER : CHUNK;

            /* reset parser_buffer_ */
            parser_buffer_ = {};
        }
        /* if you haven't seen a CRLF so far do nothing */
        return false;

    case CHUNK:
        if (parser_buffer_.length() == current_chunk_size_ + 2) {
          /* accumulated enough bytes, check CR/LF */
          assert(parser_buffer_.substr( parser_buffer_.length() - 2, 2 ) == "\r\n");

          /* Transition to next state */
          state_ = CHUNK_HDR;

          /* reset parser_buffer_ */
          parser_buffer_ = {};
        }
        return false;

    case TRAILER:
        /* If we find two consectuve CRLFs, we are done */
        return (trailers_enabled_) ? (parser_buffer_.find("\r\n\r\n") != string::npos)
                                   : (parser_buffer_.find("\r\n") != string::npos);

    default:
        assert( false );
        return false;
    }
}

string::size_type ChunkedBodyParser::read( const std::string & input_buffer )
{
  for (uint32_t i = 0; i < input_buffer.length(); i++) {
      if (parse_byte( input_buffer.at( i ) )) {
          return i + 1; /* i + 1, because it's a size, not an index */
      }
  }
  return string::npos;
}
