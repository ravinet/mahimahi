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
#include "http_disk_store.hh"
#include "process_recorder.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " directory [command...]" );
        }

        /* Make sure directory ends with '/' so we can prepend directory to file name for storage */
        string directory( argv[ 1 ] );

        if ( directory.empty() ) {
            throw Exception( argv[ 0 ], "directory name must be non-empty" );
        }

        if ( directory.back() != '/' ) {
            directory.append( "/" );
        }

        /* what command will we run inside the container? */
        vector < string > command;
        if ( argc == 2 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 2; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        MahimahiProtobufs::BulkRequest bulk_request;

        ProcessRecorder<HTTPProxy<HTTPDiskStore>> process_recorder;
        return process_recorder.record_process( [&] ( FileDescriptor & parent_channel  __attribute__ ((unused)) ) {
                                                /* restore environment and tweak prompt */
                                                environ = user_environment;
                                                prepend_shell_prefix( "[record] " );

                                                return ezexec( command, true );
                                              }, Socket( SocketType::UDP ) /* Dummy socket, unused by HTTPDiskStore */
                                               , 0
                                               , bulk_request
                                               , ""
                                               , directory );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
