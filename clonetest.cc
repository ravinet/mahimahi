#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

int bash_exec(void *) {
  printf("In child, execing bash\n");
  char* argv[1];
  argv[0] = const_cast<char*>(std::string("--login").c_str());
  execvp("bash", argv);
  return 0;           /* Child terminates now */
}

int main() {
  /* Allocate stack for child */
  char* stack = static_cast<char*>(malloc(65536));
  if (stack == nullptr) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
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
  exit(EXIT_SUCCESS);
}
