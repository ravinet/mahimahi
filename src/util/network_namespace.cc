/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "network_namespace.hh"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sched.h>

#include "config.h"
#include "system_runner.hh"
#include "exception.hh"
#include "file_descriptor.hh"

using namespace std;

NetworkNamespace::NetworkNamespace( const std::string & name )
    : name_(name), has_own_resolvconf_(false), has_entered_(false)
{
    /* create a new network namespace */
    run( { IP, "netns", "add", name_ } );
}


NetworkNamespace::~NetworkNamespace()
{
    if ( has_entered_ ) {

        char etc_netns_path[PATH_MAX];
        char netns_name[PATH_MAX];
        char etc_name[PATH_MAX];
        struct dirent *entry;
        DIR *dir;

        snprintf( etc_netns_path, sizeof( etc_netns_path ), "%s/%s", NETNS_ETC_DIR.c_str(), name_.c_str() );
        dir = opendir( etc_netns_path );
        if (!dir )
            return;

        while ( ( entry = readdir(dir) ) != NULL ) {

            if ( strcmp(entry->d_name, ".") == 0 )
                continue;
            if ( strcmp(entry->d_name, "..") == 0 )
                continue;

            snprintf(netns_name, sizeof(netns_name), "%s/%s", etc_netns_path, entry->d_name);
            snprintf(etc_name, sizeof(etc_name), "/etc/%s", entry->d_name);

            if ( umount( etc_name ) < 0 ) {
                throw runtime_error( string("Unmounting... ") + netns_name + " -> " + etc_name + " failed: " + strerror(errno) + "\n" );
            }
        }

    }

    if ( has_own_resolvconf_ ) {
        const string netns_dir = NETNS_ETC_DIR + "/" + name_;
        const string resolvconf_path = netns_dir + "/resolv.conf";

        SystemCall("unlink", unlink ( resolvconf_path.c_str() ) );
        SystemCall("unlink", rmdir ( netns_dir.c_str() ) );
    }

    /*char netns_path[PATH_MAX];
    snprintf(netns_path, sizeof(netns_path), "%s/%s", NETNS_DIR.c_str(), name_.c_str());
    umount2(netns_path, MNT_FORCE);
    if (unlink(netns_path) < 0) {
      throw runtime_error(string("Cannot remove namespace file manually \"")+netns_path+"\":" + strerror(errno) + "\n" );
    }*/


    run( { IP, "netns", "delete", name_ } );
}


void NetworkNamespace::create_resolvconf( const std::string & nameserver )
{
  has_own_resolvconf_ = true;
  /* create an individual resolv.conf for this network namespace */
  const string netns_dir = NETNS_ETC_DIR + "/" + name_;
  const string resolvconf_path = netns_dir + "/resolv.conf";

  SystemCall("mkdir", mkdir( netns_dir.c_str() , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) );

  FileDescriptor fd( SystemCall( "open", open( resolvconf_path.c_str(),
                                               O_WRONLY | O_CREAT | O_TRUNC,
                                               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) ) ) ;

  fd.write( "nameserver " + nameserver + "\n" );
}


void NetworkNamespace::enter()
{
    char net_path[PATH_MAX];
    int netns;

    snprintf( net_path, sizeof(net_path), "%s/%s", NETNS_DIR.c_str(), name_.c_str() );
    netns = open( net_path, O_RDONLY | O_CLOEXEC );

    if ( netns < 0 ) {
        throw runtime_error( "Cannot open network namespace \"" + name_ + "\": " + strerror(errno) + "\n " );
    }

    if ( setns(netns, CLONE_NEWNET) < 0 ) {
       throw runtime_error( "setting the network namespace \"" + name_ + "\" failed: " + strerror(errno) + "\n " );
    }
    close( netns );

    has_entered_ = true;


    if ( unshare(CLONE_NEWNS) < 0 ) {
        throw runtime_error( string("unshare failed: ") + strerror(errno) + "\n " );
    }

    /* Don't let any mounts propagate back to the parent */
    /*
    if ( mount( "", "/", "none", MS_SLAVE | MS_REC, NULL ) ) {
        throw runtime_error( string("\"mount --make-rslave /\" failed: ") + strerror(errno) + "\n " );
    }*/

    /* Mount a version of /sys that describes the network namespace */
    if ( umount2( "/sys", MNT_DETACH) < 0 ) {
        throw runtime_error( string("umount of /sys failed: ") + strerror(errno) + "\n" );
    }

    if ( mount( name_.c_str(), "/sys", "sysfs", 0, NULL ) < 0 ) {
        throw runtime_error( string("mount of /sys failed: ") + strerror(errno) + "\n" );
    }


    /* Setup bind mounts for config files in /etc */
    char etc_netns_path[PATH_MAX];
    char netns_name[PATH_MAX];
    char etc_name[PATH_MAX];
    struct dirent *entry;
    DIR *dir;

    snprintf( etc_netns_path, sizeof( etc_netns_path ), "%s/%s", NETNS_ETC_DIR.c_str(), name_.c_str() );
    dir = opendir( etc_netns_path );
    if (!dir )
        return;

    while ( ( entry = readdir(dir) ) != NULL ) {

        if ( strcmp(entry->d_name, ".") == 0 )
            continue;
        if ( strcmp(entry->d_name, "..") == 0 )
            continue;

        snprintf(netns_name, sizeof(netns_name), "%s/%s", etc_netns_path, entry->d_name);
        snprintf(etc_name, sizeof(etc_name), "/etc/%s", entry->d_name);

        if ( mount( netns_name, etc_name, "none", MS_BIND, NULL ) < 0 ) {
            throw runtime_error( string("Bind ") + netns_name + " -> " + etc_name + " failed: " + strerror(errno) + "\n" );
        }
    }

    closedir( dir );


}
