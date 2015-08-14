/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <paths.h>
#include <cstdlib>
#include <cassert>
#include <fstream>
#include <resolv.h>
#include <sys/stat.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <memory>
#include <numeric>

#include "util.hh"
#include "exception.hh"
#include "file_descriptor.hh"

using namespace std;

/* Get the user's shell */
string shell_path( void )
{
    passwd *pw = getpwuid( getuid() );
    if ( pw == nullptr ) {
        throw unix_error( "getpwuid" );
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
        SystemCall( "setregid", setregid( real_gid, real_gid ) );
    }

    /* change real user id if necessary */
    if ( real_uid != eff_uid ) {
        SystemCall( "setreuid", setreuid( real_uid, real_uid ) );
    }

    /* verify that the changes were successful. if not, abort */
    if ( real_gid != eff_gid && ( setegid( eff_gid ) != -1 || getegid( ) != real_gid ) ) {
        cerr << "BUG: dropping privileged gid failed" << endl;
        _exit( EXIT_FAILURE );
    }

    if ( real_uid != eff_uid && ( seteuid( eff_uid ) != -1 || geteuid( ) != real_uid ) ) {
        cerr << "BUG: dropping privileged uid failed" << endl;
        _exit( EXIT_FAILURE );
    }
}

void check_requirements( const int argc, const char * const argv[] )
{
    if ( argc <= 0 ) {
        /* really crazy user */
        throw runtime_error( "missing argv[ 0 ]: argc <= 0" );
    }

    /* verify normal fds are present (stderr hasn't been closed) */
    FileDescriptor( SystemCall( "open /dev/null", open( "/dev/null", O_RDONLY ) ) );

    /* verify running as euid root, but not ruid root */
    if ( geteuid() != 0 ) {
        throw runtime_error( string( argv[ 0 ] ) + ": needs to be installed setuid root" );
    }

    if ( (getuid() == 0) || (getgid() == 0) ) {
        throw runtime_error( string( argv[ 0 ] ) + ": please run as non-root" );
    }

    /* verify environment has been cleared */
    if ( environ ) {
        throw runtime_error( "BUG: environment not cleared in sensitive region" );
    }

    /* verify IP forwarding is enabled */
    FileDescriptor ipf( SystemCall( "open /proc/sys/net/ipv4/ip_forward",
                                    open( "/proc/sys/net/ipv4/ip_forward", O_RDONLY ) ) );
    if ( ipf.read() != "1\n" ) {
        throw runtime_error( string( argv[ 0 ] ) + ": Please run \"sudo sysctl -w net.ipv4.ip_forward=1\" to enable IP forwarding" );
    }
}

void make_directory( const string & directory )
{
    assert_not_root();
    assert( not directory.empty() );
    assert( directory.back() == '/' );

    SystemCall( "mkdir " + directory, mkdir( directory.c_str(), 00700 ) );
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
            nameservers.emplace_back( _res.nsaddr_list[ i ] );
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

vector< string > list_directory_contents( const string & dir )
{
    assert_not_root();

    struct Closedir {
        void operator()( DIR *x ) const { SystemCall( "closedir", closedir( x ) ); }
    };

    unique_ptr< DIR, Closedir > dp( opendir( dir.c_str() ) );
    if ( not dp ) {
        throw unix_error( "opendir (" + dir + ")" );
    }

    vector< string > ret;
    while ( const dirent *dirp = readdir( dp.get() ) ) {
        if ( string( dirp->d_name ) != "." and string( dirp->d_name ) != ".." ) {
            ret.push_back( dir + dirp->d_name );
        }
    }

    return ret;
}

void assert_not_root( void )
{
    if ( ( geteuid() == 0 ) or ( getegid() == 0 ) ) {
        throw runtime_error( "BUG: privileges not dropped in sensitive region" );
    }
}

TemporarilyUnprivileged::TemporarilyUnprivileged()
    : orig_euid( geteuid() ),
      orig_egid( getegid() )
{
    SystemCall( "setegid", setegid( getgid() ) );
    SystemCall( "seteuid", seteuid( getuid() ) );

    assert_not_root();
}

TemporarilyUnprivileged::~TemporarilyUnprivileged()
{
    SystemCall( "seteuid", seteuid( orig_euid ) );
    SystemCall( "setegid", setegid( orig_egid ) );
}

string join( const vector< string > & command )
{
    return accumulate( command.begin() + 1, command.end(),
                       command.front(),
                       []( const string & a, const string & b )
                       { return a + " " + b; } );
}

string get_working_directory( void )
{
    struct Free {
        void operator()( char *x ) const { free( x ); }
    };

    unique_ptr<char, Free> cwd_ptr { get_current_dir_name() };
    if ( not cwd_ptr ) {
        throw unix_error( "getcwd" );
    }

    return cwd_ptr.get();
}
