// to bring interface for tap device up --> sudo ip link set egress up
// to run arping to send packet to ingress to/from machine with ip address addr  --> arping -I ingress addr

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string>
#include <cstdlib>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

using namespace std;

int create_tap( std::string name )
{
    struct ifreq ifr;
    int fd, err;
    
    if ( ( fd = open( "/dev/net/tun", O_RDWR ) ) < 0 ) {
        perror( "open" );
        return fd;
    }
    
    // fills enough of memory area pointed to by ifr
    memset( &ifr, 0, sizeof( ifr ) );

    // specifies to create tap device
    ifr.ifr_flags = IFF_TAP; 
    

    // uses specified name for tap device
    strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ );
    
    // create interface 
    if ( ( err = ioctl( fd, TUNSETIFF, ( void * ) &ifr ) ) < 0 ){
        close( fd );
        perror( "ioctl" );
        return EXIT_FAILURE;
    }
    return fd;
}

int main ( void )
{
    // create first tap device named ingress
    int tap0_fd = create_tap( "ingress" );
    
    // create second tap device named egress
    int tap1_fd = create_tap( "egress" );
    
    // set up poll for tap devices
    struct pollfd pollfds[ 2 ];
    pollfds[ 0 ].fd = tap0_fd;
    pollfds[ 0 ].events = POLLIN;

    pollfds[ 1 ].fd = tap1_fd;
    pollfds[ 1 ].events = POLLIN;

    // poll on both tap devices
    while ( 1 ) {
        if( poll( pollfds, 2, -1 ) == -1 ) {
            perror( "poll" );
            return EXIT_FAILURE;
        }
       
        // read from ingress which triggered POLLIN
        int amount_read0;
        if ( pollfds[ 0 ].revents & POLLIN ) {
            char buf[ 16384 ];
            amount_read0  = read( pollfds[ 0 ].fd, buf, 16384 );
            if ( amount_read0 < 0 ) {
	        perror( "read" );
                return EXIT_FAILURE;
            }
            if ( amount_read0 == 0 ) {
	        cout << "nothing to read (EOF)" << endl;
	    }
            
	    // write what is read from ingress to egress after sleeping 1 second
            sleep(1);
            int written0;
            written0 = write( pollfds[ 1 ].fd, buf, amount_read0 );
            if( written0 < 0 ) {
                perror( "write" );
                return EXIT_FAILURE;
	    }
        }

        // read from egress which triggered POLLIN
        int amount_read1;
        if ( pollfds[ 1 ].revents & POLLIN ) {
            char buf1[ 16384 ];
            amount_read1  = read( pollfds[ 1 ].fd, buf1, 16384 );
            if ( amount_read1 < 0 ) {
                perror( "read" );
                return EXIT_FAILURE;
            }
            if ( amount_read1 == 0 ) {
                cout << "nothing to read (EOF)" << endl;
            }
            
            // write what is read from egress to ingress after sleeping 1 second
            sleep(1);
            int written1;
            written1 = write( pollfds[ 0 ].fd, buf1, amount_read1 );
            if( written1 < 0 ) {
                perror( "write" );
                return EXIT_FAILURE;
            }
        }
    }                     
} 
