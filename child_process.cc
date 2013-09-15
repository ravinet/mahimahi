/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <sys/wait.h>
#include <cassert>
#include <csignal>
#include <cstdlib>

#include "child_process.hh"
#include "exception.hh"

/* start up a child process running the supplied lambda */
/* the return value of the lambda is the child's exit status */
ChildProcess::ChildProcess( std::function<int()> && child_procedure )
    : pid_( fork() ),
      running_( true ),
      terminated_( false ),
      exit_status_()
{
    if ( pid_ < 0 ) {
        throw Exception( "fork" );
    }

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
    assert( !terminated_ );

    siginfo_t infop;
    if ( waitid( P_PID, pid_, &infop, WEXITED | WSTOPPED | WCONTINUED ) < 0 ) {
        throw Exception( "waitid" );
    }
    
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
        exit_status_ = EXIT_FAILURE; /* died on signal */
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
    if ( !running_ ) {
        signal( SIGCONT );
    }
}

/* send a signal to the child process */
void ChildProcess::signal( const int sig )
{
    if ( !terminated_ ) {
        if ( kill( pid_, sig ) < 0 ) {
            throw Exception( "kill" );
        }
    }
}

ChildProcess::~ChildProcess()
{
    while ( !terminated_ ) {
        /* if it's stopped, make it resume */
        if ( !running_ ) {
            resume();
        }

        wait();
    }
}
