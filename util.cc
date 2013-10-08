/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <paths.h>
#include <grp.h>
#include <cstdlib>
#include <fstream>
#include <resolv.h>

#include "util.hh"
#include "exception.hh"
#include "file_descriptor.hh"

using namespace std;

/* Get the user's shell */
string shell_path( void )
{
    passwd *pw = getpwuid( getuid() );
    if ( pw == nullptr ) {
        throw Exception( "getpwuid" );
    }

    string shell_path( pw->pw_shell );
    if ( shell_path.empty() ) { /* empty shell means Bourne shell */
      shell_path = _PATH_BSHELL;
    }

    return shell_path;
}

/* Adapted from "Secure Programming Cookbook for C and C++: Recipes for Cryptography, Authentication, Input Validation & More" - John Viega and Matt Messier */
void drop_privileges( void ) {
    gid_t real_gid = getgid( ), eff_gid = getegid( );
    uid_t real_uid = getuid( ), eff_uid = geteuid( );

    /* eliminate ancillary groups */
    if ( eff_uid == 0 ) { /* if root */
        if ( setgroups( 1, &real_gid ) == -1 ) {
            throw Exception( "setgroups" );
        }
    }

    /* change real group id if necessary */
    if ( real_gid != eff_gid ) {
        if ( setregid( real_gid, real_gid ) == -1 ) {
            throw Exception( "setregid" );
        }
    }

    /* change real user id if necessary */
    if ( real_uid != eff_uid ) {
        if ( setreuid( real_uid, real_uid ) == -1 ) {
            throw Exception( "setreuid" );
        }
    }

    /* verify that the changes were successful. if not, abort */
    if ( real_gid != eff_gid && ( setegid( eff_gid ) != -1 || getegid( ) != real_gid ) ) {
        throw Exception( "drop_privileges", "dropping gid failed" );
    }

    if ( real_uid != eff_uid && ( seteuid( eff_uid ) != -1 || geteuid( ) != real_uid ) ) {
        throw Exception( "drop_privileges", "dropping uid failed" );
    }
}

void check_requirements( const int argc, const char * const argv[] )
{
    if ( argc <= 0 ) {
        /* really crazy user */
        throw Exception( "missing argv[ 0 ]", "argc <= 0" );
    }

    /* verify normal fds are present (stderr hasn't been closed) */
    FileDescriptor( open( "/dev/null", O_RDONLY ), "open" );

    /* verify running as euid root, but not ruid root */
    if ( geteuid() != 0 ) {
        throw Exception( argv[ 0 ], "needs to be installed setuid root" );
    }

    if ( (getuid() == 0) || (getgid() == 0) ) {
        throw Exception( argv[ 0 ], "please run as non-root" );
    }

    if ( argc != 2 ) {
        throw Exception( "Usage", string( argv[ 0 ] ) + " propagation-delay [in milliseconds]" );
    }

    /* verify IP forwarding is enabled */
    FileDescriptor ipf( open( "/proc/sys/net/ipv4/ip_forward", O_RDONLY ), "open /proc/sys/net/ipv4/ip_forward" );
    if ( ipf.read() != "1\n" ) {
        throw Exception( argv[ 0 ], "Please run \"sudo sysctl -w net.ipv4.ip_forward=1\" to enable IP forwarding" );
    }
}

Address first_nameserver( void )
{
    /* find the first nameserver */
    if ( res_init() < 0 ) {
        throw Exception( "res_init" );
    }

    return _res.nsaddr;
}

/* tag bash-like shells with the delay parameter */
void prepend_shell_prefix( const uint64_t & delay_ms )
{
    const char *prefix = getenv( "MAHIMAHI_SHELL_PREFIX" );
    string mahimahi_prefix = prefix ? prefix : "";
    mahimahi_prefix.append( "[delay " + to_string( delay_ms ) + "] " );

    if ( setenv( "MAHIMAHI_SHELL_PREFIX", mahimahi_prefix.c_str(), true ) < 0 ) {
        throw Exception( "setenv" );
    }

    if ( setenv( "PROMPT_COMMAND", "PS1=\"$MAHIMAHI_SHELL_PREFIX$PS1\" PROMPT_COMMAND=", true ) < 0 ) {
        throw Exception( "setenv" );
    }
}
