/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>

#include "nat.hh"
#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "dns_proxy.hh"
#include "netdevice.hh"
#include "event_loop.hh"
#include "socketpair.hh"
#include "config.h"
#include "backing_store.hh"
#include "process_recorder.cc"
#include "http_console_store.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        /* what command will we run inside the container? */
        vector < string > command;
        if ( argc == 1 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 1; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        ProcessRecorder<LocalProxy> process_recorder;
        return process_recorder.record_process( [&] ( FileDescriptor & parent_channel  __attribute__ ((unused)) ) {
                                                /* restore environment and tweak prompt */
                                                environ = user_environment;
                                                prepend_shell_prefix( "[localproxy] " );

                                                return ezexec( command, true );
                                              }, Socket( SocketType::UDP ) /* Dummy socket, unused by HTTPDiskStore */
                                               , 0
                                               , ""
                                               , Address ( "8.8.8.8", 5555 ) );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
