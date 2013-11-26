/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "http_message.hh"
#include "http_message_sequence.hh"

using namespace std;

template <class MessageType>
bool HTTPMessageSequence<MessageType>::InternalBuffer::have_complete_line( void ) const
{
    size_t first_line_ending = buffer_.find( CRLF );
    return first_line_ending != std::string::npos;
}

template <class MessageType>
std::string HTTPMessageSequence<MessageType>::InternalBuffer::get_and_pop_line( void )
{
    size_t first_line_ending = buffer_.find( CRLF );
    assert( first_line_ending != std::string::npos );

    string first_line( buffer_.substr( 0, first_line_ending ) );
    pop_bytes( first_line_ending + CRLF.size() );

    return first_line;
}

template <class MessageType>
void HTTPMessageSequence<MessageType>::InternalBuffer::pop_bytes( const size_t num )
{
    assert( buffer_.size() >= num );
    buffer_.replace( 0, num, string() );
}

template <class MessageType>
bool HTTPMessageSequence<MessageType>::parsing_step( void )
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
            string line( buffer_.get_and_pop_line() );
            if ( line.empty() ) {
                message_in_progress_.done_with_headers();
            } else {
                message_in_progress_.add_header( line );
            }
        }
        return true;

    case BODY_PENDING:
        if ( buffer_.empty() ) { return false; }

        {
            size_t bytes_read = message_in_progress_.read_in_body( buffer_.str() );
            buffer_.pop_bytes( bytes_read );
            if ( bytes_read == 0 ) {
                assert( message_in_progress_.state() == COMPLETE );
            }
        }
        return true;
    case COMPLETE:
        complete_messages_.emplace( move( message_in_progress_ ) );
        message_in_progress_ = MessageType();
        return true;
    }

    assert( false );
    return false;
}

template <class MessageType>
void HTTPMessageSequence<MessageType>::parse( const string & buf )
{
    if ( buf.empty() ) { /* EOF */
        message_in_progress_.eof();
    }

    /* append buf to internal buffer */
    buffer_.append( buf );

    /* parse as much as we can */
    while ( parsing_step() ) {}
}
