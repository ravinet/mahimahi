/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FILE_DESCRIPTOR_HH
#define FILE_DESCRIPTOR_HH

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
    }

    ~FileDescriptor()
    {
        if ( close( fd_ ) < 0 ) {
            throw Exception( "close" );
        }
    }

    const int & num( void ) const { return fd_; }

    /* forbid copying FileDescriptor objects or assigning them */
    FileDescriptor( const FileDescriptor & other ) = delete;
    const FileDescriptor & operator=( const FileDescriptor & other ) = delete;

    void write( const std::string & buffer ) const
    {
        writeall( num(), buffer );
    }

    std::string read( void ) const
    {
        return readall( num() );
    }
};

#endif /* FILE_DESCRIPTOR_HH */
