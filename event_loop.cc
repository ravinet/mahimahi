/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "event_loop.hh"

using namespace std;
using namespace PollerShortNames;

EventLoop::EventLoop()
    : signals_( { SIGCHLD, SIGCONT, SIGHUP, SIGTERM } ),
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
            /* make sure it's from the child process */
            /* unfortunately sig.ssi_pid is a uint32_t instead of pid_t, so need to cast */
            bool handled = false;

            for ( auto & x : child_processes_ ) {
                if ( sig.ssi_pid == static_cast<decltype(sig.ssi_pid)>( x.pid() ) ) {
                    handled = true;

                    /* figure out what happened to it */
                    x.wait();

                    if ( x.terminated() ) {
                        if ( x.died_on_signal() ) {
                            throw Exception( "process " + to_string( x.pid() ),
                                             "died on signal " + to_string( x.exit_status() ) );
                        } else {
                            return Result( ResultType::Exit, x.exit_status() );
                        }
                    } else if ( !x.running() ) {
                        /* suspend parent too */
                        SystemCall( "raise", raise( SIGSTOP ) );
                    }

                    break;
                }
            }

            if ( not handled ) {
                throw Exception( "SIGCHLD for unknown process" );
            }
        }

        break;

    case SIGHUP:
    case SIGTERM:

        return ResultType::Exit;
    default:
        throw Exception( "unknown signal" );
    }

    return ResultType::Continue;
}
