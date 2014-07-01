/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_REQUEST_PARSER_HH
#define HTTP_REQUEST_PARSER_HH

#include "local_http_message_sequence.hh"
#include "local_http_request.hh"

class HTTPRequestParser : public HTTPMessageSequence<HTTPRequest>
{
private:
    void initialize_new_message( void ) override {}

public:
    void parse( const std::string & buf )
    {
        if ( buf.empty() ) { /* EOF */
            message_in_progress_.eof();
        }

        /* append buf to internal buffer */
        buffer_.append( buf );

        /* parse as much as we can */
        while ( parsing_step() ) {}
    }

    bool parsing_step( void )
    {
        switch ( message_in_progress_.state() ) {
        case FIRST_LINE_PENDING:
            /* do we have a complete line? */
            if ( not buffer_.have_complete_line() ) { return false; }

            /* supply status line to request/response initialization routine */
            initialize_new_message();

            message_in_progress_.set_first_line( buffer_.get_and_pop_line() );

            return true;
        case HEADERS_PENDING:
            /* do we have a complete line? */
            if ( not buffer_.have_complete_line() ) { return false; }

            /* is line blank? */
            {
                std::string line( buffer_.get_and_pop_line() );
                if ( line.empty() ) {
                    message_in_progress_.done_with_headers();
                } else {
                    message_in_progress_.add_header( line );
                }
            }
            return true;

        case BODY_PENDING:
            {
                size_t bytes_read = message_in_progress_.read_in_body( buffer_.str() );
                assert( bytes_read == buffer_.str().size() or message_in_progress_.state() == COMPLETE );
                buffer_.pop_bytes( bytes_read );
            }
            return message_in_progress_.state() == COMPLETE;

        case COMPLETE:
            complete_messages_.emplace( std::move( message_in_progress_ ) );
            message_in_progress_ = HTTPRequest();
            return true;
        }

        assert( false );
        return false;
    }
};

#endif /* HTTP_REQUEST_PARSER_HH */
