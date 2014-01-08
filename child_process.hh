/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CHILD_PROCESS_HH
#define CHILD_PROCESS_HH

#include <functional>
#include <unistd.h>
#include <cassert>
#include <signal.h>

/* object-oriented wrapper for handling Unix child processes */

class ChildProcess
{
private:
    pid_t pid_;
    bool running_, terminated_;
    int exit_status_;
    bool died_on_signal_;
    int graceful_termination_signal_;

    bool moved_away_;

public:
    ChildProcess( std::function<int()> && child_procedure, const bool new_namespace = false,
                  const int termination_signal = SIGHUP );

    void wait( void ); /* wait for process to change state */
    void signal( const int sig ); /* send signal */
    void resume( void ); /* send SIGCONT */

    pid_t pid( void ) const { return pid_; }
    bool running( void ) const { return running_; }
    bool terminated( void ) const { return terminated_; }

    /* Return exit status or signal that killed process */
    bool died_on_signal( void ) const { assert( terminated_ ); return died_on_signal_; }
    int exit_status( void ) const { assert( terminated_ ); return exit_status_; }

    ~ChildProcess();

    /* ban copying */
    ChildProcess( const ChildProcess & other ) = delete;
    ChildProcess & operator=( const ChildProcess & other ) = delete;

    /* allow move constructor */
    ChildProcess( ChildProcess && other );

    /* ... but not move assignment operator */
    ChildProcess & operator=( ChildProcess && other ) = delete;
};

#endif /* CHILD_PROCESS_HH */
