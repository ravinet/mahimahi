/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FILE_DESCRIPTOR_HH
#define FILE_DESCRIPTOR_HH

#include <unistd.h>
#include <fcntl.h>

#include "exception.hh"
#include "ezio.hh"

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

        if ( fd_ <= 2 ) { /* make sure not overwriting stdout/stderr */
            throw Exception( "FileDescriptor", "fd <= 2" );
        }

        /* set close-on-exec flag so our file descriptors
           aren't passed on to unrelated children (like a shell) */
        if ( fcntl( fd_, F_SETFD, FD_CLOEXEC ) < 0 ) {
            throw Exception( "fcntl FD_CLOEXEC" );
        }
    }

    ~FileDescriptor()
    {
        if ( fd_ < 0 ) { /* has already been moved away */
            return;
        }

        if ( close( fd_ ) < 0 ) {
            throw Exception( "close" );
        }
    }

    const int & num( void ) { return fd_; }

    /* forbid copying FileDescriptor objects or assigning them */
    FileDescriptor( const FileDescriptor & other ) = delete;
    const FileDescriptor & operator=( const FileDescriptor & other ) = delete;

    /* allow moving FileDescriptor objects */
    FileDescriptor( FileDescriptor && other )
        : fd_( other.fd_ )
    {
        other.fd_ = -1; /* disable the other FileDescriptor */
    }

    void write( const std::string & buffer )
    {
        writeall( num(), buffer );
    }

    std::string read( void )
    {
        return readall( num() );
    }

    std::string read( const size_t limit )
    {
        return readall( num(), limit );
    }
};

#endif /* FILE_DESCRIPTOR_HH */
