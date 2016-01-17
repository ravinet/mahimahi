/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef EVENT_LOOP_HH
#define EVENT_LOOP_HH

#include <vector>
#include <functional>

#include "poller.hh"
#include "file_descriptor.hh"
#include "signalfd.hh"
#include "child_process.hh"
#include "util.hh"

class EventLoop
{
private:
    SignalMask signals_;
    Poller poller_;
    std::vector<std::pair<int, ChildProcess>> child_processes_;
    PollerShortNames::Result handle_signal( const signalfd_siginfo & sig );

protected:
    void add_action( Poller::Action action ) { poller_.add_action( action ); }

    int internal_loop( const std::function<int(void)> & wait_time );

public:
    EventLoop();

    void add_simple_input_handler( FileDescriptor & fd, const Poller::Action::CallbackType & callback );

    template <typename... Targs>
    void add_child_process( Targs&&... Fargs )
    {
        child_processes_.emplace_back( -1,
                                       ChildProcess( std::forward<Targs>( Fargs )... ) );
    }

    template <typename... Targs>
    void add_special_child_process( const int continue_status,
                                    Targs&&... Fargs )
    {
        /* parent won't quit when this process quits */
        child_processes_.emplace_back( continue_status, ChildProcess( std::forward<Targs>( Fargs )... ) );
    }

    int loop( void ) { return internal_loop( [] () { return -1; } ); } /* no timeout */

    virtual ~EventLoop() {}
};

#endif /* EVENT_LOOP_HH */
