/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// to run arping to send packet to ingress to/from machine with ip address addr  --> arping -I ingress addr

#include <sys/socket.h>
#include <pwd.h>
#include <paths.h>
#include <fstream>

#include "tapdevice.hh"
#include "exception.hh"
#include "ferry.hh"
#include "child_process.hh"

using namespace std;

/* get name of the user's shell */
string shell_path( void )
{
    struct passwd *pw = getpwuid( getuid() );
    if ( pw == nullptr ) {
        throw Exception( "getpwuid" );
    }

    string shell_path( pw->pw_shell );
    if ( shell_path.empty() ) { /* empty shell means Bourne shell */
      shell_path = _PATH_BSHELL;
    }

    return shell_path;
}

int main( void )
{
    try {
        ifstream input;
        input.open("/proc/sys/net/ipv4/ip_forward");
        string line;
        getline( input, line );
	if ( line != "1" ) {
            throw Exception( "ip forwarding disabled" );
        }

        /* make pair of connected sockets */
        int pipes[ 2 ];
        if ( socketpair( AF_UNIX, SOCK_DGRAM, 0, pipes ) < 0 ) {
            throw Exception( "socketpair" );
        }
        FileDescriptor egress_socket( pipes[ 0 ], "socketpair" ),
            ingress_socket( pipes[ 1 ], "socketpair" );

        /* Fork */
        ChildProcess container_process( [&]()->int{
                /* Unshare network namespace */
                if ( unshare( CLONE_NEWNET ) == -1 ) {
                    throw Exception( "unshare" );
                }

                TapDevice ingress_tap( "ingress" );

                /* Fork again */
                ChildProcess bash_process( []()->int{
                        const string shell = shell_path();
                        if ( execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) < 0 ) {
                            throw Exception( "execl" );
                        }
                        return EXIT_FAILURE;
                    } );

                return ferry( ingress_tap.fd(), egress_socket, bash_process, 2500 );
            } );

        TapDevice egress_tap( "egress" );
        return ferry( egress_tap.fd(), ingress_socket, container_process, 2500 );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    } catch ( const ChildProcess::Exit & e ) {
        return e.status();
    }

    return EXIT_SUCCESS;
}
