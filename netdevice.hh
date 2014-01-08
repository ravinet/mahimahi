/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef NETDEVICE_HH
#define NETDEVICE_HH

#include <string>
#include <functional>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#include "file_descriptor.hh"
#include "address.hh"

/* general helpers */
void interface_ioctl( FileDescriptor & fd, const int request,
                      const std::string & name,
                      std::function<void( ifreq &ifr )> ifr_adjustment);

void assign_address( const std::string & device_name, const Address & addr, const Address & peer );

class TunDevice
{
private:
    FileDescriptor fd_;
public:
    TunDevice( const std::string & name, const Address & addr, const Address & peer );

    FileDescriptor & fd( void ) { return fd_; }
};

class VirtualEthernetPair
{
private:
    std::string name;

public:
    VirtualEthernetPair( const std::string & s_outside_name, const std::string & s_inside_name );
    ~VirtualEthernetPair();
};

#endif /* NETDEVICE_HH */
