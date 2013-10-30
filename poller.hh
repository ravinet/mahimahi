/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef POLLER_HH
#define POLLER_HH

#include <functional>
#include <vector>
#include <cassert>

#include <poll.h>

#include "file_descriptor.hh"

class Poller
{
public:
    struct Action
    {
        struct Result
        {
            enum class Type { Continue, Exit } result;
            unsigned int exit_status;
            Result( const Type & s_result = Type::Continue, const unsigned int & s_status = EXIT_SUCCESS )
                : result( s_result ), exit_status( s_status ) {}
        };

        typedef std::function<Result(void)> CallbackType;

        FileDescriptor fd;
        enum PollDirection : short { In = POLLIN, Out = POLLOUT } direction;
        CallbackType callback;

        Action( FileDescriptor && s_fd,
                const PollDirection & s_direction,
                const CallbackType & s_callback )
            : fd( std::move( s_fd ) ), direction( s_direction ), callback( s_callback ) {}
    };

private:
    std::vector< pollfd > pollfds_;
    std::vector< Action::CallbackType > callbacks_;

public:
    struct Result
    {
        enum class Type { Success, Timeout, Exit } result;
        unsigned int exit_status;
        Result( const Type & s_result, const unsigned int & s_status = EXIT_SUCCESS )
            : result( s_result ), exit_status( s_status ) {}
    };

    Poller() : pollfds_(), callbacks_() {}
    void add_action( Action & action );
    Result poll( const int & timeout_ms );
};

#endif
