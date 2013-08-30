/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

// Unshares network namespace and executes bash shell
// By Ravi Netravali <ravinet@mit.edu>

#include <sched.h>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string.h>

using namespace std;

int main ( void )
{
    // make array of character for exec system call { bash, 0}
    char *argv[2];
    string cmd = "bash";
    argv[0] = const_cast<char*> (cmd.c_str());
    argv[1] = 0;

    // unshares network namespace (prints whether success or failure)
    if ( unshare( CLONE_NEWNET ) == -1 ) {
        perror( "unshare( CLONE_NEWNET )" );
        return EXIT_FAILURE;
    } else {
        cout << "Success in unsharing" << endl;
        // executes bash shell
        if ( execv( "/bin/bash", argv ) == -1 ) {
            perror( "execv" );
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
