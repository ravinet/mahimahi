#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <assert.h>
#include <iostream>

using namespace std;

int bash_exec(void *) {
  printf("In child, execing bash\n");
  char* argv[2];
  argv[0] = const_cast<char*>("bash"); /* generally program name, or "-bash" for login shell */
  argv[1] = nullptr; /* terminate argv */
  if ( execvp("bash", argv) < 0 ) {
    perror( "execvp" );
    return EXIT_FAILURE;
  }
  assert( false ); /* If execvp() successful, we never get here */
  return EXIT_FAILURE;
}

int main() {
  /* Allocate stack for child */
  char* stack = new char [ 65536 ];
  /* will throw an exception if fails -- no need to check return value */
  char* stack_top = stack + 65536;  /* Stack grows downward */

  /* Clone child */
  pid_t pid = clone(bash_exec, stack_top, CLONE_NEWNET | SIGCHLD, nullptr);
  if (pid == -1) {
    perror("clone");
    exit(EXIT_FAILURE);
  }
  printf("clone() returned %ld\n", (long) pid);

  /* Wait for child */
  int status;
  if (wait(&status) == -1) {
    perror("wait");
    exit(EXIT_FAILURE);
  }

  printf("child has terminated\n");

  delete[] stack; /* should free memory we allocated earlier */

  exit(EXIT_SUCCESS);
}
