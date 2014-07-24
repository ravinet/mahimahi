/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef EVENT_LOOP_HH
#define EVENT_LOOP_HH

#include <vector>
#include <functional>
#include <utility>

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
    std::vector<std::pair<ChildProcess, bool>> child_processes_;
    PollerShortNames::Result handle_signal( const signalfd_siginfo & sig );

protected:
    void add_action( Poller::Action action ) { poller_.add_action( action ); }

    int internal_loop( const std::function<int(void)> & wait_time );

public:
    EventLoop();

    void add_simple_input_handler( FileDescriptor & fd, const Poller::Action::CallbackType & callback );

    void add_child_process( ChildProcess && child_process, bool exit_when_terminated = true )
    {
        child_processes_.emplace_back( std::make_pair( std::move( child_process ), exit_when_terminated ) );
    }

    int loop( void ) { return internal_loop( [] () { return -1; } ); } /* no timeout */

    virtual ~EventLoop() {}
};

#endif /* EVENT_LOOP_HH */
