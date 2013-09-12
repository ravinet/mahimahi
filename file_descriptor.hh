/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FILE_DESCRIPTOR_HH
#define FILE_DESCRIPTOR_HH

#include "exception.hh"

class FileDescriptor
{
private:
    int fd_;

public:
    FileDescriptor( const int s_fd, const std::string & syscall_name )
        : fd_( s_fd )
    {
        if ( fd_ < 0 ) {
            throw Exception( syscall_name );
        }
    }

    ~FileDescriptor()
    {
        if ( close( fd_ ) < 0 ) {
            throw Exception( "close" );
        }
    }

    const int & fd( void ) const { return fd_; }
};

#endif /* FILE_DESCRIPTOR_HH */
