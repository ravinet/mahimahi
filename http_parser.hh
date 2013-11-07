#ifndef HTTP_PARSER_HH
#define HTTP_PARSER_HH

#include <vector>
#include <string>

class HTTPHeader
{
private:
  std::string key_, value_;

public:
  HTTPHeader( const std::string & buf );

  std::string key( void ) const { return key_; }
  std::string value( void ) const { return value_; }
};

class HTTPParser
{
private:
  std::string internal_buffer_;

  std::string request_line_;
  std::vector< HTTPHeader > headers_;

  bool headers_finished_;

  size_t body_left_;

  size_t body_len( void ) const;

  std::string current_request_;

public:
  HTTPParser() : internal_buffer_(),
		       request_line_(),
		       headers_(),
		       headers_finished_( false ),
		       body_left_( 0 ),
                       current_request_()
  {}

  bool parse( const std::string & buf );
  bool headers_parsed( void ) const { return headers_finished_; }

  std::string get_header_value( const std::string & header_name ) const;
  bool has_header( const std::string & header_name ) const;

  std::string get_current_request( void );

  void reset_current_request( void ) { current_request_.clear(); }

  const std::string & request_line( void ) const { return request_line_; }
};

#endif /* HTTP_PARSER_HH */
