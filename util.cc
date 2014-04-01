/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <paths.h>
#include <grp.h>
#include <cstdlib>
#include <fstream>
#include <resolv.h>
#include <sys/stat.h>
#include <dirent.h>
#include <arpa/inet.h>

#include "util.hh"
#include "exception.hh"
#include "file_descriptor.hh"
#include "poller.hh"

using namespace std;
using namespace PollerShortNames;

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
    FileDescriptor( SystemCall( "open /dev/null", open( "/dev/null", O_RDONLY ) ) );

    /* verify running as euid root, but not ruid root */
    if ( geteuid() != 0 ) {
        throw Exception( argv[ 0 ], "needs to be installed setuid root" );
    }

    if ( (getuid() == 0) || (getgid() == 0) ) {
        throw Exception( argv[ 0 ], "please run as non-root" );
    }

    /* verify IP forwarding is enabled */
    FileDescriptor ipf( SystemCall( "open /proc/sys/net/ipv4/ip_forward",
                                    open( "/proc/sys/net/ipv4/ip_forward", O_RDONLY ) ) );
    if ( ipf.read() != "1\n" ) {
        throw Exception( argv[ 0 ], "Please run \"sudo sysctl -w net.ipv4.ip_forward=1\" to enable IP forwarding" );
    }
}

bool check_folder_existence( const string & directory )
{
    struct stat sb;

    /* check if directory already exists */
    if ( !stat( directory.c_str(), &sb ) == 0 or !S_ISDIR( sb.st_mode ) )
    {
        if ( errno != ENOENT ) { /* error is not that directory does not exist */
            throw Exception( "stat" );
        }
        return false;
    }

    return true;
}

void check_storage_folder( const string & directory )
{
    /* assert that directory ends with '/' */
    assert( directory.back() == '/' );

    if ( not check_folder_existence( directory ) ) { /* directory does not exist */
        /* make directory where user has all permissions */
        SystemCall( "mkdir", mkdir( directory.c_str(), 00700 ) );
    } else { /* directory already exists */
        throw Exception( "recordshell", "directory already exists" );
    }
}

Address first_nameserver( void )
{
    /* find the first nameserver */
    SystemCall( "res_init", res_init() );
    return _res.nsaddr;
}

vector< Address > all_nameservers( void )
{
    SystemCall( "res_init", res_init() );

    vector< Address > nameservers;

    /* iterate through the nameservers */
    for ( unsigned int i = 0; i < MAXNS; i++ ) {
        if ( _res.nsaddr_list[ i ].sin_port ) {
            nameservers.emplace_back( Address( inet_ntoa( _res.nsaddr_list[ i ].sin_addr ), ntohs( _res.nsaddr_list[ i ].sin_port ) ) );
        }
    }
    return nameservers;
}

/* tag bash-like shells with the delay parameter */
void prepend_shell_prefix( const string & str )
{
    const char *prefix = getenv( "MAHIMAHI_SHELL_PREFIX" );
    string mahimahi_prefix = prefix ? prefix : "";
    mahimahi_prefix.append( str );

    SystemCall( "setenv", setenv( "MAHIMAHI_SHELL_PREFIX", mahimahi_prefix.c_str(), true ) );
    SystemCall( "setenv", setenv( "PROMPT_COMMAND", "PS1=\"$MAHIMAHI_SHELL_PREFIX$PS1\" PROMPT_COMMAND=", true ) );
}

Result handle_signal( const signalfd_siginfo & sig,
                      vector<ChildProcess> & child_processes )
{
    switch ( sig.ssi_signo ) {
    case SIGCONT:
        /* resume child processes too */
        for ( auto & x : child_processes ) {
            x.resume();
        }
        break;

    case SIGCHLD:
        {
            /* make sure it's from the child process */
            /* unfortunately sig.ssi_pid is a uint32_t instead of pid_t, so need to cast */
            bool handled = false;

            for ( auto & x : child_processes ) {
                if ( sig.ssi_pid == static_cast<decltype(sig.ssi_pid)>( x.pid() ) ) {
                    handled = true;

                    /* figure out what happened to it */
                    x.wait();

                    if ( x.terminated() ) {
                        if ( x.died_on_signal() ) {
                            throw Exception( "process " + to_string( x.pid() ),
                                             "died on signal " + to_string( x.exit_status() ) );
                        } else {
                            return Result( ResultType::Exit, x.exit_status() );
                        }
                    } else if ( !x.running() ) {
                        /* suspend parent too */
                        SystemCall( "raise", raise( SIGSTOP ) );
                    }

                    break;
                }
            }

            if ( not handled ) {
                throw Exception( "SIGCHLD for unknown process" );
            }
        }

        break;

    case SIGHUP:
    case SIGTERM:

        return ResultType::Exit;
    default:
        throw Exception( "unknown signal" );
    }

    return ResultType::Continue;
}

void list_files( const string & dir, vector< string > & files )
{
    DIR *dp;
    struct dirent *dirp;

    if( ( dp  = opendir( dir.c_str() ) ) == NULL ) {
        throw Exception( "opendir" );
    }

    while ( ( dirp = readdir( dp ) ) != NULL ) {
        if ( string( dirp->d_name ) != "." and string( dirp->d_name ) != ".." ) {
            files.push_back( dir + string( dirp->d_name ) );
        }
    }
    SystemCall( "closedir", closedir( dp ) );
}

/* error-checking wrapper for most syscalls */
int SystemCall( const string & s_attempt, const int return_value )
{
  if ( return_value >= 0 ) {
    return return_value;
  }

  throw Exception( s_attempt );
}

void assert_not_root( void )
{
    if ( ( getuid() == 0 ) or ( geteuid() == 0 )
         or ( getgid() == 0 ) or ( getegid() == 0 ) ) {
        throw Exception( "BUG", "privileges not dropped in sensitive region" );
    }
}
