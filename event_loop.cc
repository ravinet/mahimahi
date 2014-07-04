/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "event_loop.hh"

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
        }

        break;

    case SIGHUP:
    case SIGTERM:
    case SIGQUIT:
    case SIGINT:
        return ResultType::Exit;
    default:
        throw Exception( "EventLoop", "unknown signal" );
    }

    return ResultType::Continue;
}
