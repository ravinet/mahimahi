/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <sys/wait.h>
#include <unistd.h>
#include <cassert>
#include <iostream>

using namespace std;

void bash_exec( void ) {
  /* Unshare network namespace */
  if ( unshare( CLONE_NEWNET ) == -1 ) {
    perror( "unshare( CLONE_NEWNET )" );
    exit( EXIT_FAILURE );
  }

  /* Run bash */
  char* argv[ 2 ] = { const_cast<char *>( "bash" ), /* can use "-bash" for login shell */
                      nullptr };

  if ( execvp( "bash", argv ) < 0 ) {
    perror( "execvp" );
    exit( EXIT_FAILURE );
  }

  assert( false ); /* We never get here */
}

int main( void ) {
  /* Fork child */
  pid_t pid = fork();
  if ( pid == -1 ) {
    perror( "fork" );
    exit( EXIT_FAILURE );
  } else if ( pid == 0 ) { /* child */
    bash_exec(); /* doesn't return */
  }

  /* parent */
  cerr << "fork() returned " << pid << endl;

  /* Wait for child to exit */
  int status;
  if ( waitpid( pid, &status, 0 ) == -1) {
    perror( "waitpid" );
    exit( EXIT_FAILURE );
  }

  /* Report status */
  if ( WIFEXITED( status ) ) {
    cerr << "child exited with status " << WEXITSTATUS( status ) << "." << endl;
    exit( WEXITSTATUS( status ) );
  } else if ( WIFSIGNALED( status ) ) {
    cerr << "child was terminated by signal " << WTERMSIG( status ) << "." << endl;
    exit( EXIT_FAILURE );
  }

  cerr << "child state changed for unknown reason." << endl;
  exit( EXIT_FAILURE );
}
