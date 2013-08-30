// Unshares network namespace and executes bash shell
// By Ravi Netravali <ravinet@mit.edu>

#include <sched.h>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

using namespace std;

int main ( void )
{
    // make array of character for exec system call { bash, 0}
    char *argv[2];
    string cmd = "bash";
    argv[0] = const_cast<char*> (cmd.c_str());
    argv[1] = 0;

    // unshares network namespace (prints whether success or failure)
    if ( unshare( CLONE_NEWNET ) == -1 )
    {
        cout << "Error in unsharing" << endl;
    }
    else
    {
        cout << "Success in unsharing" << endl;
        // executes bash shell
        execv("/bin/bash", argv);
    }
    return EXIT_SUCCESS;
}
