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
#include <memory>

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

vector< string > list_directory_contents( const string & dir )
{
    assert_not_root();

    struct Closedir {
        void operator()( DIR *x ) const { SystemCall( "closedir", closedir( x ) ); }
    };

    unique_ptr< DIR, Closedir > dp( opendir( dir.c_str() ) );
    if ( not dp ) {
        throw Exception( "opendir (" + dir + ")" );
    }

    vector< string > ret;
    while ( const dirent *dirp = readdir( dp.get() ) ) {
        if ( string( dirp->d_name ) != "." and string( dirp->d_name ) != ".." ) {
            ret.push_back( dir + dirp->d_name );
        }
    }

    return ret;
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
    if ( ( geteuid() == 0 ) or ( getegid() == 0 ) ) {
        throw Exception( "BUG", "privileges not dropped in sensitive region" );
    }
}

/* general function for uid->username or gid->groupname */
template <typename entry_type, typename numeric_type>
static string nssname( const int property,
                       const function<numeric_type(void)> & get_numeric,
                       const function<int(numeric_type, entry_type *, char *, size_t, entry_type **) > & retrieve_entry,
                       const function<string(const entry_type &)> & get_name )
{
    const long entry_size = sysconf( property );
    if ( entry_size <= 0 ) {
        throw Exception( "sysconf", "bad size" );
    }

    unique_ptr<char[]> buffer( new char[ entry_size ] );
    entry_type entry;
    entry_type *result;

    SystemCall( "nssname retrieve_entry",
                retrieve_entry( get_numeric(), &entry, buffer.get(), entry_size, &result ) );

    if ( result == nullptr ) {
        throw Exception( "nssname", "no matching record was found" );
    }

    if ( result != &entry ) {
        throw Exception( "nssname", "BUG: unexpected result" );
    }

    return get_name( entry );
}

string username( void )
{
    return nssname<passwd, uid_t>( _SC_GETPW_R_SIZE_MAX, getuid, getpwuid_r,
                                   [] ( const passwd & x ) { return x.pw_name; } );
}

string groupname( void )
{
    return nssname<group, gid_t>( _SC_GETGR_R_SIZE_MAX, getgid, getgrgid_r,
                                  [] ( const group & x ) { return x.gr_name; } );
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
