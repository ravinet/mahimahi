/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_PARSER_HH
#define HTTP_PARSER_HH

#include <vector>
#include <string>
#include <queue>

#include "http_header.hh"
#include "http_request.hh"

class HTTPParser
{
private:
  std::string internal_buffer_;

  std::string request_line_;
  std::vector< HTTPHeader > headers_;

  bool headers_finished_;

  std::string body_;

  size_t body_left_;

  size_t body_len( void ) const;

  std::queue< HTTPRequest > complete_requests_;

public:
  HTTPParser() : internal_buffer_(),
		       request_line_(),
		       headers_(),
		       headers_finished_( false ),
                       body_(),
		       body_left_( 0 ),
                       complete_requests_()
  {}

  void parse( const std::string & buf );

  bool headers_parsed( void ) const { return headers_finished_; }

  std::string get_header_value( const std::string & header_name ) const;

  bool has_header( const std::string & header_name ) const;

  const std::string & request_line( void ) const { return request_line_; }

  bool empty( void ) const { return complete_requests_.empty(); }

  HTTPRequest get_request( void );

};

#endif /* HTTP_PARSER_HH */
