/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef SIGNALFD_HH
#define SIGNALFD_HH

#include <sys/signalfd.h>
#include <initializer_list>

#include "file_descriptor.hh"

/* wrapper class for Unix signal masks and signal file descriptor */

class SignalMask
{
private:
    sigset_t mask_;

public:
    SignalMask( const std::initializer_list< int > signals );
    const sigset_t & mask( void ) const { return mask_; }
    void block( void ) const;
};

class SignalFD
{
private:
    FileDescriptor fd_;

public:
    SignalFD( const SignalMask & signals );

    const FileDescriptor & fd( void ) const { return fd_; }
    signalfd_siginfo read_signal( void ) const; /* read one signal */
};

#endif /* SIGNALFD_HH */
