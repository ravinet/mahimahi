/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_MESSAGE_SEQUENCE
#define HTTP_MESSAGE_SEQUENCE

#include <string>
#include <queue>

template <class MessageType>
class HTTPMessageSequence
{
private:
    class InternalBuffer
    {
    private:
        std::string buffer_ {};
    
    public:
        bool have_complete_line( void ) const;

        std::string get_and_pop_line( void );

        void pop_bytes( const size_t n );

        bool empty( void ) const { return buffer_.empty(); }

        void append( const std::string & str ) { buffer_.append( str ); }

        const std::string & str( void ) const { return buffer_; }
    };

    /* bytes that haven't been parsed yet */
    InternalBuffer buffer_ {};

    /* complete messages ready to go */
    std::queue< MessageType > complete_messages_ {};

    /* one loop through the parser */
    /* returns whether to continue */
    bool parsing_step( void );

    /* what to do to create a new message.
       must be implemented by subclass */
    virtual void initialize_new_message( void ) = 0;

protected:
    /* the current message we're working on */
    MessageType message_in_progress_ {};

public:
    HTTPMessageSequence() {}

    /* must accept all of buf */
    void parse( const std::string & buf );

    /* getters */
    bool empty( void ) const { return complete_messages_.empty(); }
    const MessageType & front( void ) const { return complete_messages_.front(); }

    /* pop one request */
    void pop( void ) { complete_messages_.pop(); }
};

#endif /* HTTP_MESSAGE_SEQUENCE */
