// Unshares network namespace and executes bash shell
// By Ravi Netravali <ravinet@mit.edu>
// Compile with: g++ -std=c++0x -g -O2 -Wall -Wextra -Weffc++ -Werror -pedantic -o unsharenet unsharenet.cc

#include <sched.h>
#include <iostream>
#include <cstdlib>

using namespace std;

int main ( void )
{
    if ( unshare( CLONE_NEWNET ) == -1 )
    {
        cout << "Error in unsharing" << endl;
    }
    else
    {
        cout << "Success in unsharing" << endl;
    }
    return EXIT_SUCCESS;
}
