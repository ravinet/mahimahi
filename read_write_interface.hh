/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef READ_WRITE_INTERFACE_HH
#define READ_WRITE_INTERFACE_HH

#include <string>
#include "file_descriptor.hh"

/*
   Common Read/Write Interface shared
   between Socket and SecureSocket
 */
class ReadWriteInterface
{
public:
    virtual std::string read( void ) = 0;
    virtual void write( const std::string & str ) = 0;
    virtual FileDescriptor & fd( void ) = 0;

    virtual ~ReadWriteInterface() {}
};

#endif
