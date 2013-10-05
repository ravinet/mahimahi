/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef TUNDEVICE_HH
#define TUNDEVICE_HH

#include <string>
#include <functional>

#include "file_descriptor.hh"

class TunDevice
{
private:
    FileDescriptor fd_;
public:
    TunDevice( const std::string & name, const std::string & addr, const std::string & dstaddr );

    FileDescriptor & fd( void ) { return fd_; }

    static void interface_ioctl( const int fd, const int request,
                                 const std::string & name,
                                 std::function<void( struct ifreq &ifr )> ifr_adjustment);
};

#endif /* TUNDEVICE_HH */
