/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CHILD_PROCESS_HH
#define CHILD_PROCESS_HH

#include <functional>
#include <unistd.h>
#include <cassert>

/* object-oriented wrapper for handling Unix child processes */

class ChildProcess
{
private:
    pid_t pid_;
    bool running_, terminated_;
    int exit_status_;

public:
    ChildProcess( std::function<int()> && child_procedure );

    void wait( void ); /* wait for process to change state */
    void signal( const int sig ); /* send signal */
    void resume( void ); /* send SIGCONT */

    pid_t pid( void ) const { return pid_; }
    bool running( void ) const { return running_; }
    bool terminated( void ) const { return terminated_; }

    /* Return exit status, or general FAILURE if died on signal */
    bool exit_status( void ) const { assert( terminated_ ); return exit_status_; }

    ~ChildProcess();
};

#endif /* CHILD_PROCESS_HH */
