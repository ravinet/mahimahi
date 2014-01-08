/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <sys/wait.h>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <sys/syscall.h>

#include "child_process.hh"
#include "exception.hh"

/* start up a child process running the supplied lambda */
/* the return value of the lambda is the child's exit status */
ChildProcess::ChildProcess( std::function<int()> && child_procedure, const bool new_namespace,
                            const int termination_signal )
    : pid_( SystemCall( "clone", syscall( SYS_clone,
                                          SIGCHLD | (new_namespace ? CLONE_NEWNET : 0),
                                          nullptr, nullptr, nullptr, nullptr ) ) ),
      running_( true ),
      terminated_( false ),
      exit_status_(),
      died_on_signal_( false ),
      graceful_termination_signal_( termination_signal ),
      moved_away_( false )
{
    if ( pid_ == 0 ) { /* child */
        try {
            _exit( child_procedure() );
        } catch ( const Exception & e ) {
            e.perror();
            _exit( EXIT_FAILURE );
        }
    }
}

/* wait for process to change state */
void ChildProcess::wait( void )
{
    assert( !moved_away_ );
    assert( !terminated_ );

    siginfo_t infop;
    SystemCall( "waitid", waitid( P_PID, pid_, &infop, WEXITED | WSTOPPED | WCONTINUED ) );
    
    assert( infop.si_pid == pid_ );
    assert( infop.si_signo == SIGCHLD );    

    /* how did the process change state? */
    switch ( infop.si_code ) {
    case CLD_EXITED:
        terminated_ = true;
        exit_status_ = infop.si_status;
        break;
    case CLD_KILLED:
    case CLD_DUMPED:
        terminated_ = true;
        exit_status_ = infop.si_status; /* died on signal */
        died_on_signal_ = true;
        break;
    case CLD_STOPPED:
        running_ = false;
        break;
    case CLD_CONTINUED:
        running_ = true;
        break;
    default:
        /* do nothing */
        break;
    }
}

/* if child process was suspended, resume it */
void ChildProcess::resume( void )
{
    assert( !moved_away_ );

    if ( !running_ ) {
        signal( SIGCONT );
    }
}

/* send a signal to the child process */
void ChildProcess::signal( const int sig )
{
    assert( !moved_away_ );

    if ( !terminated_ ) {
        SystemCall( "kill", kill( pid_, sig ) );
    }
}

ChildProcess::~ChildProcess()
{
    if ( moved_away_ ) { return; }

    while ( !terminated_ ) {
        resume();
        signal( graceful_termination_signal_ );
        wait();
    }
}

/* move constructor */
ChildProcess::ChildProcess( ChildProcess && other )
    : pid_( other.pid_ ),
      running_( other.running_ ),
      terminated_( other.terminated_ ),
      exit_status_( other.exit_status_ ),
      died_on_signal_( other.died_on_signal_ ),
      graceful_termination_signal_( other.graceful_termination_signal_ ),
      moved_away_( other.moved_away_ )
{
    assert( !other.moved_away_ );

    other.moved_away_ = true;
}
