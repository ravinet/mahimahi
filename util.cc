#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <paths.h>
#include <grp.h>

#include "util.hh"
#include "exception.hh"

using namespace std;

/* Get the user's shell */
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

void drop_privileges( void ) {
    gid_t real_gid = getgid( ), eff_gid = getegid( );
    uid_t real_uid = getuid( ), eff_uid = geteuid( );

    /* eliminate ancillary groups */
    if ( eff_uid == 0 ) { /* if root */
        setgroups( 1, &real_gid );
    }

    /* change real group id if necessary */
    if ( real_gid != eff_gid ) {
        if ( setregid( real_gid, real_gid ) == -1 ) {
            abort( );
        }
    }

    /* change real user id if necessary */
    if ( real_uid != eff_uid ) {
        if ( setreuid( real_uid, real_uid ) == -1 ) {
            abort( );
        }
    }

    /* verify that the changes were successful. if not, abort */
    if ( real_gid != eff_gid && ( setegid( eff_gid ) != -1 || getegid( ) != real_gid ) ) {
        abort( );
    }

    if ( real_uid != eff_uid && ( seteuid( eff_uid ) != -1 || geteuid( ) != real_uid ) ) {
        abort( );
    }
}
