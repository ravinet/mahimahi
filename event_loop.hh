/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef EVENT_LOOP_HH
#define EVENT_LOOP_HH

#include <vector>

#include "poller.hh"
#include "file_descriptor.hh"
#include "signalfd.hh"
#include "child_process.hh"

class EventLoop
{
private:
    SignalMask signals_;
    Poller poller_;
    std::vector<ChildProcess> child_processes_;
    PollerShortNames::Result handle_signal( const signalfd_siginfo & sig );

protected:
    void add_action( Poller::Action action ) { poller_.add_action( action ); }

    template <typename LambdaType>
    int internal_loop( const LambdaType & wait_time )
    {
        /* verify that signal mask is intact */
        SignalMask current_mask = SignalMask::current_mask();

        if ( !( signals_ == current_mask ) ) {
            throw Exception( "EventLoop", "signal mask has been altered" );
        }

        SignalFD signal_fd( signals_ );

        /* we get signal -> main screen turn on */
        add_simple_input_handler( signal_fd.fd(),
                                  [&] () { return handle_signal( signal_fd.read_signal() ); } );

        while ( true ) {
            const auto poll_result = poller_.poll( wait_time() );
            if ( poll_result.result == Poller::Result::Type::Exit ) {
                return poll_result.exit_status;
            }
        }
    }

public:
    EventLoop();

    void add_simple_input_handler( FileDescriptor & fd, const Poller::Action::CallbackType & callback );

    template <typename... Targs>
    void add_child_process( Targs&&... Fargs )
    {
        child_processes_.emplace_back( std::forward<Targs>( Fargs )... );
    }

    int loop( void ) { return internal_loop( [] () { return -1; } ); } /* no timeout */

    virtual ~EventLoop() {}
};

#endif /* EVENT_LOOP_HH */
