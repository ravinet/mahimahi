/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FINITE_QUEUE_HH
#define FINITE_QUEUE_HH

#include <string>
#include <queue>
#include <assert.h>

class FiniteQueue
{
public:
    struct QueuedPacket
    {
        int bytes_to_transmit;
        std::string contents;
        QueuedPacket( const std::string & s_contents );
    };

private:
    std::queue< QueuedPacket > finite_queue_;

    unsigned int queue_size_;

    unsigned int max_size_;
public:
    FiniteQueue( signed int max_size );

    void emplace( const std::string & contents );

    bool empty( void ) const;

    QueuedPacket & front( void );

    void pop( void );

};

#endif /* FINITE_QUEUE_HH */
