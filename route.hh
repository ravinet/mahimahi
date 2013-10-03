/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ROUTE_HH
#define ROUTE_HH

#include <linux/if.h>
#include <arpa/inet.h>
#include <net/route.h>

/* Setup default route */

class Route
{
public:
  Route( FileDescriptor sockfd, std::string default_gw )
  {
    /* http://blogs.nologin.es/rickyepoderi/index.php?/archives/8-Setting-Routes-Using-C.html */
    // set route struct to zero
    struct rtentry route;
    struct sockaddr_in *addr;
    int err = 0;
    memset(&route, 0, sizeof(route));

    // assign the default gateway
    addr = reinterpret_cast<struct sockaddr_in*>( &route.rt_gateway );
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(default_gw.c_str());

    // assign the destination
    addr = reinterpret_cast<struct sockaddr_in*>( &route.rt_dst );
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("0.0.0.0");

    // assign the destination mask
    addr = reinterpret_cast<struct sockaddr_in*>( &route.rt_genmask );
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("0.0.0.0");

    // set the flags NET/UP
    route.rt_flags = RTF_UP | RTF_GATEWAY;
    // make the ioctl
    if ((err = ioctl(sockfd.num(), SIOCADDRT, &route)) != 0) {
       perror("SIOCADDRT failed");
       exit(-1);
    }
  }
};

#endif /* NAT_HH */
