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
    bool eof_;

    unsigned int read_count_, write_count_;

protected:
    void register_read( void ) { read_count_++; }
    void register_write( void ) { write_count_++; }

public:
    FileDescriptor( const int s_fd )
        : fd_( s_fd ), eof_( false ),
          read_count_(), write_count_()
    {
        if ( fd_ <= 2 ) { /* make sure not overwriting stdout/stderr */
            throw unix_error( "FileDescriptor: fd <= 2" );
        }

        /* set close-on-exec flag so our file descriptors
           aren't passed on to unrelated children (like a shell) */
        SystemCall( "fcntl FD_CLOEXEC", fcntl( fd_, F_SETFD, FD_CLOEXEC ) );
    }

    virtual ~FileDescriptor()
    {
        if ( fd_ < 0 ) { /* has already been moved away */
            return;
        }

        try {
            SystemCall( "close", close( fd_ ) );
        } catch ( const std::exception & e ) { /* don't throw from destructor */
            print_exception( e );
        }
    }

    const int & num( void ) { return fd_; }
    const bool & eof( void ) const { return eof_; }

    /* forbid copying FileDescriptor objects or assigning them */
    FileDescriptor( const FileDescriptor & other ) = delete;
    const FileDescriptor & operator=( const FileDescriptor & other ) = delete;

    /* allow moving FileDescriptor objects */
    FileDescriptor( FileDescriptor && other )
        : fd_( other.fd_ ), eof_( other.eof_ ),
          read_count_( other.read_count_ ),
          write_count_( other.write_count_)
    {
        other.fd_ = -1; /* disable the other FileDescriptor */
    }

    void write( const std::string & buffer )
    {
        writeall( num(), buffer );

        register_write();
    }

    std::string::const_iterator write_some( const std::string::const_iterator & begin,
                                            const std::string::const_iterator & end )
    {
        return ::write_some( num(), begin, end );
    }

    std::string read( void )
    {
        auto ret = readall( num() );
        if ( ret.empty() ) { eof_ = true; }

        register_read();

        return ret;
    }

    std::string read( const size_t limit )
    {
        auto ret = readall( num(), limit );
        if ( ret.empty() ) { eof_ = true; }

        register_read();

        return ret;
    }

    void set_eof( void )
    {
        eof_ = true;
    }

    unsigned int read_count( void ) const { return read_count_; }
    unsigned int write_count( void ) const { return write_count_; }
};

#endif /* FILE_DESCRIPTOR_HH */
