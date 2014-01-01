/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>
#include <iostream>
#include <vector>
#include <dirent.h>
#include <set>

#include "util.hh"
#include "get_address.hh"
#include "address.hh"
#include "signalfd.hh"
#include "netdevice.hh"
#include "web_server.hh"
#include "system_runner.hh"
#include "config.h"
#include "socket.hh"
#include "config.h"
#include "poller.hh"
#include "http_record.pb.h"

using namespace std;

/*
Find request in environment variables
for each request, compare to the stored requests until we find a match
(can have a function which takes two requests and compares them while ignoring the 
time-sensitive headers)
Then, once we find a match, we will write the response (potentially without the time
sensitive headers).
*/

int main()
{
    try {
        extern char **environ;
        cout << "Content-type: text/html\r\n\r\n";
        for(char **current = environ; *current; current++) {
            cout << *current << endl << "<br />\n";
        }
        cout << "THIS IS THE REQUEST_URI: " << getenv( "REQUEST_URI" ) << "<br />\n" << endl; 
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
