/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef TAPDEVICE_HH
#define TAPDEVICE_HH

#include <string>

#include "file_descriptor.hh"

class TapDevice
{
private:
    FileDescriptor fd_;
public:
    TapDevice( const std::string & name, const std::string & addr, const std::string & dstaddr );

    const FileDescriptor & fd( void ) const { return fd_; }
};

#endif /* TAPDEVICE_HH */
