/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef POLLER_HH
#define POLLER_HH

#include <functional>
#include <vector>
#include <cassert>

#include <sys/epoll.h>

#include "file_descriptor.hh"

class Poller
{
public:
    struct Action
    {
        struct Result
        {
            enum class Type { Continue, Exit, Cancel } result;
            unsigned int exit_status;
            Result( const Type & s_result = Type::Continue, const unsigned int & s_status = EXIT_SUCCESS )
                : result( s_result ), exit_status( s_status ) {}
        };

        typedef std::function<Result(void)> CallbackType;

        FileDescriptor & fd;
        enum PollDirection : short { In = EPOLLIN, Out = EPOLLOUT } direction;
        CallbackType callback;
        std::function<bool(void)> when_interested;
        bool active;

        Action( FileDescriptor & s_fd,
                const PollDirection & s_direction,
                const CallbackType & s_callback,
                const std::function<bool(void)> & s_when_interested = [] () { return true; } )
            : fd( s_fd ), direction( s_direction ), callback( s_callback ),
              when_interested( s_when_interested ), active( true ) {}

        epoll_event to_epoll_event( const uint32_t index ) const;
    };

private:
    FileDescriptor epoll_fd_;

    std::vector< Action > actions_;
    std::vector< epoll_event > events_;

public:
    struct Result
    {
        enum class Type { Success, Timeout, Exit } result;
        unsigned int exit_status;
        Result( const Type & s_result, const unsigned int & s_status = EXIT_SUCCESS )
            : result( s_result ), exit_status( s_status ) {}
    };

    Poller();
    void add_action( Action action );
    Result poll( const int & timeout_ms );
};

namespace PollerShortNames {
    typedef Poller::Action::Result Result;
    typedef Poller::Action::Result::Type ResultType;
    typedef Poller::Action::PollDirection Direction;
}

#endif
