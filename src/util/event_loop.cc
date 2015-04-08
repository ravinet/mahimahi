/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "event_loop.hh"
#include "exception.hh"

using namespace std;
using namespace PollerShortNames;

EventLoop::EventLoop()
    : signals_( { SIGCHLD, SIGCONT, SIGHUP, SIGTERM, SIGQUIT, SIGINT } ),
      poller_(),
      child_processes_()
{
    signals_.set_as_mask(); /* block signals so we can later use signalfd to read them */
}

void EventLoop::add_simple_input_handler( FileDescriptor & fd,
                                          const Poller::Action::CallbackType & callback )
{
    poller_.add_action( Poller::Action( fd, Direction::In, callback ) );
}

Result EventLoop::handle_signal( const signalfd_siginfo & sig )
{
    switch ( sig.ssi_signo ) {
    case SIGCONT:
        /* resume child processes too */
        for ( auto & x : child_processes_ ) {
            x.resume();
        }
        break;

    case SIGCHLD:
        {
            /* find which children are waitable */
            /* we can't count on getting exactly one SIGCHLD per waitable event, so search */
            for ( auto & x : child_processes_ ) {
                if ( x.waitable() ) {
                    x.wait( true ); /* nonblocking */

                    if ( x.terminated() ) {
                        if ( x.exit_status() != 0 ) {
                            x.throw_exception();
                        } else {
                            return ResultType::Exit;
                        }
                    } else if ( !x.running() ) {
                        /* suspend parent too */
                        SystemCall( "raise", raise( SIGSTOP ) );
                    }

                    break;
                }
            }
        }

        break;

    case SIGHUP:
    case SIGTERM:
    case SIGQUIT:
    case SIGINT:
        return ResultType::Exit;
    default:
        throw runtime_error( "EventLoop: unknown signal" );
    }

    return ResultType::Continue;
}

int EventLoop::internal_loop( const std::function<int(void)> & wait_time )
{
    TemporarilyUnprivileged tu;

    /* verify that signal mask is intact */
    SignalMask current_mask = SignalMask::current_mask();

    if ( !( signals_ == current_mask ) ) {
        throw runtime_error( "EventLoop: signal mask has been altered" );
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
